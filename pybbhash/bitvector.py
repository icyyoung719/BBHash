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

    # save/load binary interface compatible with C++
    def save(self, os):
        """Save to binary stream (file opened in 'wb' mode).
        Compatible with C++ bitVector::save format."""
        import struct
        # Write _size (uint64_t)
        os.write(struct.pack("<Q", self._size))
        # Write _nchar (uint64_t)
        os.write(struct.pack("<Q", self._nchar))
        # Write _bitArray (array of uint64_t)
        for w in self._bitArray:
            os.write(struct.pack("<Q", w & ((1 << 64) - 1)))
        # Write ranks size (uint64_t)
        os.write(struct.pack("<Q", len(self._ranks)))
        # Write _ranks (array of uint64_t)
        for r in self._ranks:
            os.write(struct.pack("<Q", r))

    @staticmethod
    def load(is_stream):
        """Load from binary stream (file opened in 'rb' mode).
        Compatible with C++ bitVector::load format.
        Returns a new bitvector instance."""
        import struct
        bv = bitvector(0)
        # Read _size (uint64_t)
        data = is_stream.read(8)
        if not data:
            return bv
        bv._size = struct.unpack("<Q", data)[0]
        # Read _nchar (uint64_t)
        bv._nchar = struct.unpack("<Q", is_stream.read(8))[0]
        # Read _bitArray
        bv._bitArray = []
        for _ in range(bv._nchar):
            w = struct.unpack("<Q", is_stream.read(8))[0]
            bv._bitArray.append(w)
        # Read ranks size
        sizer = struct.unpack("<Q", is_stream.read(8))[0]
        # Read _ranks
        bv._ranks = []
        for _ in range(sizer):
            r = struct.unpack("<Q", is_stream.read(8))[0]
            bv._ranks.append(r)
        return bv
