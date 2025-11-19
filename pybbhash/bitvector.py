"""Simple bitvector with rank support (single-threaded).

This is a minimal port of the C++ `bitVector` used by BooPHF. It's not optimized
for memory or speed; it aims to preserve the semantics needed by the MPH builder.
"""
from typing import List

WORDSZ = 64


def popcount64(x: int) -> int:
    # Python's bit_count works for arbitrary ints (Python 3.8+ has int.bit_count)
    return x.bit_count()


class bitvector:
    def __init__(self, nbits: int = 0):
        self._size = int(nbits)
        # number of 64-bit words
        self._nchar = 1 + (self._size // WORDSZ) if self._size > 0 else 0
        # underlying array of python ints (represent 64-bit words)
        self._bitArray: List[int] = [0] * self._nchar
        # ranks sampling array (same idea as C++ implementation)
        self._nb_bits_per_rank_sample = 512
        self._ranks: List[int] = []

    def resize(self, newsize: int):
        self._size = int(newsize)
        self._nchar = 1 + (self._size // WORDSZ) if self._size > 0 else 0
        self._bitArray = [0] * self._nchar

    def size(self) -> int:
        return self._size

    def clear(self):
        for i in range(self._nchar):
            self._bitArray[i] = 0

    def get(self, pos: int) -> int:
        if pos < 0 or pos >= self._size:
            return 0
        return (self._bitArray[pos >> 6] >> (pos & 63)) & 1

    # name preserved: non-atomic test-and-set (single-threaded)
    def atomic_test_and_set(self, pos: int) -> int:
        mask = 1 << (pos & 63)
        idx = pos >> 6
        old = (self._bitArray[idx] >> (pos & 63)) & 1
        self._bitArray[idx] |= mask
        return old

    def get64(self, cell64: int) -> int:
        return self._bitArray[cell64]

    def set(self, pos: int):
        idx = pos >> 6
        self._bitArray[idx] |= (1 << (pos & 63))

    def reset(self, pos: int):
        idx = pos >> 6
        self._bitArray[idx] &= ~(1 << (pos & 63))

    def bitSize(self) -> int:
        # bits used by array + ranks (approx)
        return self._nchar * WORDSZ + len(self._ranks) * WORDSZ

    def build_ranks(self, offset: int = 0) -> int:
        # compute sampled ranks per _nb_bits_per_rank_sample
        self._ranks = []
        cur_rank = offset
        for i in range(self._nchar):
            if ((i * WORDSZ) % self._nb_bits_per_rank_sample) == 0:
                self._ranks.append(cur_rank)
            cur_rank += popcount64(self._bitArray[i])
        return cur_rank

    def rank(self, pos: int) -> int:
        if pos >= self._size:
            pos = self._size - 1
        word_idx = pos // WORDSZ
        word_offset = pos % WORDSZ
        block = pos // self._nb_bits_per_rank_sample
        r = 0
        if block < len(self._ranks):
            r = self._ranks[block]
        else:
            r = 0
        start_word = (block * self._nb_bits_per_rank_sample) // WORDSZ
        for w in range(start_word, word_idx):
            r += popcount64(self._bitArray[w])
        mask = (1 << word_offset) - 1 if word_offset > 0 else 0
        r += popcount64(self._bitArray[word_idx] & mask)
        return r

    # save/load simple binary interface (optional)
    def save(self, f):
        # write size and words as little-endian ints
        import struct
        f.write(struct.pack("<Q", self._size))
        f.write(struct.pack("<Q", self._nchar))
        for w in self._bitArray:
            f.write(struct.pack("<Q", w & ((1 << 64) - 1)))
        f.write(struct.pack("<Q", len(self._ranks)))
        for r in self._ranks:
            f.write(struct.pack("<Q", r))

    def load(self, f):
        import struct
        data = f.read(8 * 2)
        if not data:
            return
        self._size, self._nchar = struct.unpack("<QQ", data)
        self._bitArray = []
        for _ in range(self._nchar):
            wdata = f.read(8)
            (w,) = struct.unpack("<Q", wdata)
            self._bitArray.append(w)
        (sizer,) = struct.unpack("<Q", f.read(8))
        self._ranks = []
        for _ in range(sizer):
            (r,) = struct.unpack("<Q", f.read(8))
            self._ranks.append(r)
