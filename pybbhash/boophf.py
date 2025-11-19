"""Single-threaded BooPHF-like MPH builder in Python.

This is a simplified port intended for correctness and clarity rather than speed.
"""
from typing import Iterable, Dict, List, Tuple
from .bitvector import bitvector
from .hashfunctors import XorshiftHashFunctors, SingleHashFunctor
import math

hash_pair_t = Tuple[int, int]


def fastrange64(word: int, p: int) -> int:
    if p == 0:
        return 0
    return word % p


class level:
    def __init__(self, idx_begin=0, hash_domain=0):
        self.idx_begin = idx_begin
        self.hash_domain = hash_domain
        self.bitset = bitvector(hash_domain)

    def get(self, hash_raw: int) -> int:
        hashi = fastrange64(hash_raw, self.hash_domain)
        return self.bitset.get(hashi)


class mphf:
    def __init__(self, n: int = 0, input_range: Iterable[int] = None, num_thread: int = 1, gamma: float = 2.0, writeEach: bool = False, progress: bool = False, perc_elem_loaded: float = 0.03):
        # single-threaded implementation: ignore num_thread
        self._gamma = gamma
        self._hash_domain = int(math.ceil(float(n) * gamma)) if n > 0 else 0
        self._nelem = int(n)
        self._num_thread = 1
        self._percent_elem_loaded_for_fastMode = perc_elem_loaded
        self._withprogress = progress
        self._fastmode = False
        self._writeEachLevel = writeEach
        self._final_hash: Dict[int, int] = {}
        self._levels: List[level] = []
        self._nb_levels = 0
        self._hasher = XorshiftHashFunctors(SingleHashFunctor())
        self._lastbitsetrank = 0

        if self._nelem == 0 or input_range is None:
            self._built = False
            return

        if self._percent_elem_loaded_for_fastMode > 0.0:
            self._fastmode = True

        self.setup()

        offset = 0
        for ii in range(self._nb_levels):
            self._tempBitset = bitvector(self._levels[ii].hash_domain)
            # process level
            self.processLevel(input_range, ii)
            # clear collisions
            self._levels[ii].bitset.clear()
            self._levels[ii].bitset.clear()  # no-op replacement for C++ clearCollisions semantics
            offset = self._levels[ii].bitset.build_ranks(offset)
            del self._tempBitset

        self._lastbitsetrank = offset
        self._built = True

    def setup(self):
        self._cptTotalProcessed = 0
        if self._fastmode:
            # pre-allocate setLevelFastmode size
            self.setLevelFastmode = [0] * int(self._percent_elem_loaded_for_fastMode * float(self._nelem))
        else:
            self.setLevelFastmode = []

        self._proba_collision = 1.0 - pow(((self._gamma * float(self._nelem) - 1) / (self._gamma * float(self._nelem))), max(1, self._nelem - 1))
        sum_geom = self._gamma * (1.0 + self._proba_collision / (1.0 - self._proba_collision)) if self._proba_collision < 1.0 else self._gamma

        self._nb_levels = 25
        self._levels = [level() for _ in range(self._nb_levels)]

        previous_idx = 0
        for ii in range(self._nb_levels):
            self._levels[ii].idx_begin = previous_idx
            hd = int(math.ceil(self._hash_domain * pow(self._proba_collision, ii)))
            # round up to multiple of 64
            hd = ((hd + 63) // 64) * 64
            if hd == 0:
                hd = 64
            self._levels[ii].hash_domain = hd
            previous_idx += hd

        self._fastModeLevel = 0
        for ii in range(self._nb_levels):
            if pow(self._proba_collision, ii) < self._percent_elem_loaded_for_fastMode:
                self._fastModeLevel = ii
                break

    def getLevel(self, val: int, maxlevel: int = 100, minlevel: int = 0) -> Tuple[int, int]:
        # returns (level, last_hash)
        level_idx = 0
        hash_raw = 0
        s = [0, 0]
        for ii in range(min(self._nb_levels - 1, maxlevel)):
            if ii == 0:
                hash_raw = self._hasher.h0(s, val)
            elif ii == 1:
                hash_raw = self._hasher.h1(s, val)
            else:
                hash_raw = self._hasher.next(s)

            if ii >= minlevel and self._levels[ii].get(hash_raw):
                break
            level_idx += 1
        return level_idx, hash_raw

    def lookup(self, elem: int) -> int:
        if not self._built:
            return -1
        level_idx, level_hash = self.getLevel(elem)
        if level_idx == self._nb_levels - 1:
            if elem not in self._final_hash:
                return -1
            return self._final_hash[elem] + self._lastbitsetrank
        non_minimal = fastrange64(level_hash, self._levels[level_idx].hash_domain)
        return self._levels[level_idx].bitset.rank(non_minimal)

    def processLevel(self, input_range: Iterable[int], i: int):
        # allocate the bitset for this level
        self._levels[i].bitset = bitvector(self._levels[i].hash_domain)
        # simple single-threaded scan
        writebuff: List[int] = []
        writebuff_sz = 0
        cpt = 0
        for val in input_range:
            lvl, _ = self.getLevel(val, i if self._writeEachLevel else i)
            if lvl == i:
                if self._fastmode and i == self._fastModeLevel:
                    if len(self.setLevelFastmode) > 0:
                        idx = len(self.setLevelFastmode)
                        # append (simple behavior)
                        self.setLevelFastmode.append(val)
                if i == self._nb_levels - 1:
                    # final hash
                    self._final_hash[val] = len(self._final_hash)
                else:
                    # compute next hash and set bit
                    # here we simply compute h0/h1/next relative to val
                    s = [0, 0]
                    if lvl == 0:
                        level_hash = self._hasher.h0(s, val)
                    elif lvl == 1:
                        level_hash = self._hasher.h1(s, val)
                    else:
                        level_hash = self._hasher.next(s)
                    hashl = fastrange64(level_hash, self._levels[i].hash_domain)
                    if self._levels[i].bitset.atomic_test_and_set(hashl):
                        # if collision, set in temp bitset
                        self._tempBitset.atomic_test_and_set(hashl)
            cpt += 1
            if self._withprogress and (cpt & 1023) == 0:
                pass
        self._cptLevel = cpt

    def nbKeys(self) -> int:
        return self._nelem

    def totalBitSize(self) -> int:
        totalsizeBitset = sum(l.bitset.bitSize() for l in self._levels)
        totalsize = totalsizeBitset + len(self._final_hash) * 42 * 8
        return totalsize

    def save(self, fpath: str):
        """Save mphf to binary file compatible with C++ format."""
        import struct
        with open(fpath, "wb") as os:
            # Write _gamma (double, 8 bytes)
            os.write(struct.pack("<d", self._gamma))
            # Write _nb_levels (uint32_t, 4 bytes)
            os.write(struct.pack("<I", self._nb_levels))
            # Write _lastbitsetrank (uint64_t, 8 bytes)
            os.write(struct.pack("<Q", self._lastbitsetrank))
            # Write _nelem (uint64_t, 8 bytes)
            os.write(struct.pack("<Q", self._nelem))
            
            # Save each level's bitset
            for ii in range(self._nb_levels):
                self._levels[ii].bitset.save(os)
            
            # Save final hash
            final_hash_size = len(self._final_hash)
            os.write(struct.pack("<Q", final_hash_size))
            
            for key, value in self._final_hash.items():
                # Write key (int/uint64_t, 8 bytes)
                os.write(struct.pack("<Q", key))
                # Write value (uint64_t, 8 bytes)
                os.write(struct.pack("<Q", value))

    @staticmethod
    def load(fpath: str):
        """Load mphf from binary file compatible with C++ format."""
        import struct
        mph = mphf()
        
        with open(fpath, "rb") as is_stream:
            # Read _gamma (double)
            mph._gamma = struct.unpack("<d", is_stream.read(8))[0]
            # Read _nb_levels (uint32_t)
            mph._nb_levels = struct.unpack("<I", is_stream.read(4))[0]
            # Read _lastbitsetrank (uint64_t)
            mph._lastbitsetrank = struct.unpack("<Q", is_stream.read(8))[0]
            # Read _nelem (uint64_t)
            mph._nelem = struct.unpack("<Q", is_stream.read(8))[0]
            
            # Load each level's bitset
            mph._levels = []
            for ii in range(mph._nb_levels):
                lv = level()
                lv.bitset = bitvector.load(is_stream)
                mph._levels.append(lv)
            
            # Mini setup: recompute size of each level (same as C++)
            mph._proba_collision = 1.0 - pow(((mph._gamma * float(mph._nelem) - 1) / (mph._gamma * float(mph._nelem))), max(1, mph._nelem - 1))
            previous_idx = 0
            mph._hash_domain = int(math.ceil(float(mph._nelem) * mph._gamma))
            
            for ii in range(mph._nb_levels):
                mph._levels[ii].idx_begin = previous_idx
                hd = int(math.ceil(mph._hash_domain * pow(mph._proba_collision, ii)))
                hd = ((hd + 63) // 64) * 64
                if hd == 0:
                    hd = 64
                mph._levels[ii].hash_domain = hd
                previous_idx += hd
            
            # Restore final hash
            mph._final_hash = {}
            final_hash_size = struct.unpack("<Q", is_stream.read(8))[0]
            
            for ii in range(final_hash_size):
                # Read key (uint64_t)
                key = struct.unpack("<Q", is_stream.read(8))[0]
                # Read value (uint64_t)
                value = struct.unpack("<Q", is_stream.read(8))[0]
                mph._final_hash[key] = value
            
            mph._built = True
            mph._hasher = XorshiftHashFunctors(SingleHashFunctor())
            mph._num_thread = 1
            mph._fastmode = False
            mph._withprogress = False
            mph._writeEachLevel = False
        
        return mph
