"""pybbhash - a minimal single-threaded port of BooPHF (BBHash) in Python.

This package implements a simplified, single-threaded minimal perfect hash builder
in pure Python. It's intended as a reference/portable implementation, not a high-performance
replacement for the C++ original.

Modules:
- bitvector: bit array with rank support
- hashfunctors: simple hash functors used by boophf
- boophf: single-threaded MPH builder (BooPHF-like)

Usage example:
    from pybbhash import boophf
    keys = [10, 20, 30]
    mph = boophf.mphf(len(keys), keys)
    idx = mph.lookup(20)

"""

from .bitvector import bitvector
from .hashfunctors import SingleHashFunctor, XorshiftHashFunctors
from .boophf import mphf

__all__ = ["bitvector", "SingleHashFunctor", "XorshiftHashFunctors", "mphf"]
