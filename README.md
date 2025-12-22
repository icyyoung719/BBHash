[![Build Status](https://travis-ci.org/rizkg/BBHash.svg?branch=master)](https://travis-ci.org/rizkg/BBHash)

# Cross-Platform Changes
[BBhash](https://github.com/rizkg/BBHash) is not cross-platform. It only compiles and works on Linux. So I have forked it and made it cross-platform.

Now this fork can compile on Windows (both x86 and x64) and Linux (x64, also supports x86).

This is a simple description of the build process:

1. Read raw data from a file with consistent (platform-independent) order (e.g., avoid using unordered_set). As long as the key order is consistent, the generated mphf will be the same across platforms.
2. Build BBhash and output .mphf file
3. Query BBhash (from building raw or loading mphf file) with a key

Cross-Platform ensures two things:
1. The mphf files are all the same on multiple platforms
2. The mphf files can be loaded on both platforms

So you can build mphf on one platform, and load/query mphf on **ANY** other platform.

**For detailed information about cross-platform compatibility, see [CROSS_PLATFORM.md](CROSS_PLATFORM.md).**


# BBHash
BBHash is a simple library for building minimal perfect hash function.
It is designed to handle large scale datasets. The function is just a little bit larger than other state-of-the-art libraries, it takes approximately 3 bits / elements (compared to 2.62 bits/elem for the emphf lib), but construction is faster and does not require additional memory. 

It is easy to include in other projects (just include a single .h file) and has no dependencies.

# Citation

A. Limasset, G. Rizk, R. Chikhi, P. Peterlongo, _Fast and Scalable Minimal Perfect Hashing for Massive Key Sets_, SEA 2017: http://drops.dagstuhl.de/opus/volltexte/2017/7619/pdf/LIPIcs-SEA-2017-25.pdf

```
@InProceedings{bbhash,
  author ={Antoine Limasset and Guillaume Rizk and Rayan Chikhi and Pierre Peterlongo},
  title ={{Fast and Scalable Minimal Perfect Hashing for Massive Key Sets}},
  booktitle ={16th International Symposium on Experimental Algorithms (SEA 2017)},
  pages ={25:1--25:16},
  series ={Leibniz International Proceedings in Informatics (LIPIcs)},
  year ={2017},
  volume ={75},
  ISSN ={1868-8969},
  publisher ={Schloss Dagstuhl--Leibniz-Zentrum fuer Informatik},
  address ={Dagstuhl, Germany},
  URL ={http://drops.dagstuhl.de/opus/volltexte/2017/7619},
  doi ={10.4230/LIPIcs.SEA.2017.25}
}
```

# Usage
Here is a simple example showing how to build and query a mphf with input keys in a std::vector<uint64_t> . BBHash is mainly designed for de-duplicated input. Keys can be read from a disk file, or from some user-defined iterator.

     #include "BooPHF.h"
     //tells bbhash to use included hash function working on uint64_t input keys :
     typedef boomphf::SingleHashFunctor<uint64_t>  hasher_t;
     typedef boomphf::mphf<  uint64_t, hasher_t  > boophf_t;
     
    std::vector<uint64_t> input_keys;
    //
    ... fill the input_keys vector
    //build the mphf  
    boophf_t * bphf = new boomphf::mphf<uint64_t,hasher_t>(input_keys.size(),input_keys,nthreads);
     
     //query the mphf :
     uint64_t  idx = bphf->lookup(input_keys[0]);

# Types supported
The master branch works with Plain Old Data types only (POD). To work with other types, use the "alltypes" branch (it runs slighlty slower). The alltypes branch includes a sample code with strings. The "internal_hash" branch allows to work with types that do not support copy or assignment operators, at the expense of using 128bits/key in I/O operations regardless of the actual key size. Thus, if your keys are 64 bits integers, "internal_hash" will do twice more I/Os. But if your keys are longer than 128 bits, then "internal_hash" branch will be faster than the master branch.

# How to build and run

The project uses CMake as the build system. To build the project:

    mkdir build
    cd build
    cmake ..
    cmake --build .
    
This will build the example executables and tests in the `build/` directory.

## Running examples

Example executables are located in the `examples/` directory and can be run after building:

    # Run the basic example (params are nb_elements nb_threads)
    ./example 100000000 1
    
    # Run the custom hash example
    ./example_custom_hash 100000000 1
    
    # Run the main example
    ./main

## Running tests

Test executables are located in the `tests/` directory and can be run after building:

    # Run the endianness test (cross-platform serialization)
    ./test_endian
    
    # Run the minimal test (requires specific CSV file)
    ./test_min

## Project structure

- `include/` - Header files (BooPHF.h, bitvector.hpp, etc.)
- `examples/` - Example programs demonstrating library usage
- `tests/` - Test programs
- `build/` - CMake build directory (created when building)

# Original BBHash documentation

File bootest.cpp contains more options, use ./bootest with -check to check correctness of the hash function, and -bench to benchmark lookup performance (note: currently has compilation issues).
    
Here is a sample output :
    


    ./Bootest 100000000 8 1.0 -check -bench
    key file generated 
    Construct a BooPHF with  100000000 elements  
    for info, total work write each  : 3.718    total work inram from level 8 : 9.408  total work raw : 25.000 
    [Building BooPHF]  100  %   elapsed:   0 min 10 sec   remaining:   0 min 0  sec
    BooPHF constructed perfect hash for 100000000 keys in 10.25s
    Bitarray       305808384  bits (100.00 %)   (array + ranks )
    Last level hash             0  bits (0.00 %) (nb in last level hash 0)
    boophf  bits/elem : 3.058084
     --- boophf working correctly --- 
    bench lookups  sample size 10000000 
    BBhash bench lookups average 243.84 ns +- stddev  13.01 %   (fingerprint 4999580507664480)
 

# Querying keys that are not in the input set

If you query a key that is not in the input set of keys, then one of the two following events will happen: 

1. BBHash tells you that the key is not in the set (by returning `ULLONG_MAX`), or 
     
2. BBHash returns the index of a completely different key. 
     
This is actually the behavior of  any minimal perfect hash function, not just BBHash. It is just how they work. Need to increase the probability of situation 1 happening? Increase the `gamma` parameter (at the expense of memory usage), or store a signature of your key at the position given by the index. Or to completely prevent situation 2, store the whole key at its index, but this somewhat defeats the purpose of having a minimal perfect hash.

# Authors
Guillaume Rizk, Antoine Limasset, Rayan Chikhi

guillaume.rizk@algorizk.com
