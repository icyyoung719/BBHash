"""Hash functors ported from BooPHF C++ code.

This provides a SingleHashFunctor (wrapper around a simple hash64) and
an Xorshift-based multi-hasher that yields multiple pseudo-random hashes
from two seeds (h0 and h1).
"""
from typing import List


class HashFunctors:
    # minimal seed table generation similar to C++ version
    MAXNBFUNC = 10

    def __init__(self):
        self._nbFct = 7
        self._user_seed = 0
        self._seed_tab = [0] * self.MAXNBFUNC
        self.generate_hash_seed()

    @staticmethod
    def _hash64(key: int, seed: int) -> int:
        # port of the small non-cryptographic hash from the C++ code
        hashv = seed & 0xFFFFFFFFFFFFFFFF
        k = key & 0xFFFFFFFFFFFFFFFF
        hashv ^= (hashv << 7) & 0xFFFFFFFFFFFFFFFF
        hashv ^= (k * (hashv >> 3)) & 0xFFFFFFFFFFFFFFFF
        hashv ^= (~((hashv << 11) + (k ^ (hashv >> 5)))) & 0xFFFFFFFFFFFFFFFF
        hashv = (~hashv) + ((hashv << 21) & 0xFFFFFFFFFFFFFFFF)
        hashv ^= (hashv >> 24)
        hashv = (hashv + (hashv << 3) + (hashv << 8)) & 0xFFFFFFFFFFFFFFFF
        hashv ^= (hashv >> 14)
        hashv = (hashv + (hashv << 2) + (hashv << 4)) & 0xFFFFFFFFFFFFFFFF
        hashv ^= (hashv >> 28)
        hashv = (hashv + (hashv << 31)) & 0xFFFFFFFFFFFFFFFF
        return hashv

    def generate_hash_seed(self):
        rbase = [
            0xAAAAAAAA55555555, 0x33333333CCCCCCCC, 0x6666666699999999, 0xB5B5B5B54B4B4B4B,
            0xAA55AA5555335533, 0x33CC33CCCC66CC66, 0x6699669999B599B5, 0xB54BB54B4BAA4BAA,
            0xAA33AA3355CC55CC, 0x33663366CC99CC99
        ]
        for i in range(self.MAXNBFUNC):
            self._seed_tab[i] = rbase[i]
        for i in range(self.MAXNBFUNC):
            self._seed_tab[i] = (self._seed_tab[i] * self._seed_tab[(i + 3) % self.MAXNBFUNC] + self._user_seed) & 0xFFFFFFFFFFFFFFFF

    def hashWithSeed(self, key: int, seed: int) -> int:
        return self._hash64(key, seed)

    def __call__(self, key: int) -> List[int]:
        hset = [0] * self.MAXNBFUNC
        for i in range(self.MAXNBFUNC):
            hset[i] = self._hash64(key, self._seed_tab[i])
        return hset


class SingleHashFunctor:
    def __init__(self):
        self._hf = HashFunctors()

    def __call__(self, key: int, seed: int = 0xAAAAAAAA55555555) -> int:
        return self._hf.hashWithSeed(key, seed)


class XorshiftHashFunctors:
    """Generate multiple hashes using xorshift state seeded from single-hasher."""

    def __init__(self, single_hasher=None):
        self.single_hasher = single_hasher or SingleHashFunctor()

    def h0(self, s: List[int], key: int) -> int:
        s[0] = self.single_hasher(key, 0xAAAAAAAA55555555)
        return s[0]

    def h1(self, s: List[int], key: int) -> int:
        s[1] = self.single_hasher(key, 0x33333333CCCCCCCC)
        return s[1]

    def next(self, s: List[int]) -> int:
        s1 = s[0]
        s0 = s[1]
        s[0] = s0
        s1 ^= (s1 << 23) & 0xFFFFFFFFFFFFFFFF
        s[1] = (s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26)) & 0xFFFFFFFFFFFFFFFF
        return (s[1] + s0) & 0xFFFFFFFFFFFFFFFF

    def __call__(self, key: int):
        s = [0, 0]
        hset = [0] * 10
        hset[0] = self.single_hasher(key, 0xAAAAAAAA55555555)
        hset[1] = self.single_hasher(key, 0x33333333CCCCCCCC)
        s[0], s[1] = hset[0], hset[1]
        for ii in range(2, 10):
            hset[ii] = self.next(s)
        return hset
