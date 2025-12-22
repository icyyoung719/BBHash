#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <cstdint>
#include <vector>
#include "endian_utils.hpp"

namespace boomphf {

/// Efficient popcount for 32-bit integers
[[nodiscard]] inline constexpr uint32_t popcount_32(uint32_t x) noexcept {
    constexpr uint32_t m1 = 0x55555555;
    constexpr uint32_t m2 = 0x33333333;
    constexpr uint32_t m4 = 0x0f0f0f0f;
    constexpr uint32_t h01 = 0x01010101;
    
    x -= (x >> 1) & m1;
    x = (x & m2) + ((x >> 2) & m2);
    x = (x + (x >> 4)) & m4;
    return (x * h01) >> 24;
}

/// Efficient popcount for 64-bit integers
[[nodiscard]] inline constexpr uint64_t popcount_64(uint64_t x) noexcept {
    const uint32_t low = x & 0xffffffff;
    const uint32_t high = (x >> 32ULL) & 0xffffffff;
    return popcount_32(low) + popcount_32(high);
}

/// Concurrent bit vector with atomic operations and rank support
class bitVector {
public:
    bitVector() = default;
    
    explicit bitVector(uint64_t n) : _size(n) {
        _nchar = 1ULL + n / 64ULL;
        _bitArray = new std::atomic<uint64_t>[_nchar]();
    }

    ~bitVector() {
        delete[] _bitArray;
    }

    // Copy constructor
    bitVector(const bitVector& r) : _size(r._size), _nchar(r._nchar), _ranks(r._ranks) {
        _bitArray = new std::atomic<uint64_t>[_nchar];
        for (size_t i = 0; i < _nchar; ++i) {
            _bitArray[i].store(r._bitArray[i].load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
    }

    // Copy assignment operator
    bitVector& operator=(const bitVector& r) {
        if (&r != this) {
            _size = r._size;
            _nchar = r._nchar;
            _ranks = r._ranks;

            delete[] _bitArray;
            _bitArray = new std::atomic<uint64_t>[_nchar];
            for (size_t i = 0; i < _nchar; ++i) {
                _bitArray[i].store(r._bitArray[i].load(std::memory_order_relaxed), std::memory_order_relaxed);
            }
        }
        return *this;
    }

    // Move assignment operator
    bitVector& operator=(bitVector&& r) noexcept {
        if (&r != this) {
            delete[] _bitArray;

            _size = r._size;
            _nchar = r._nchar;
            _ranks = std::move(r._ranks);
            _bitArray = r._bitArray;
            r._bitArray = nullptr;
            r._size = 0;
        }
        return *this;
    }
    
    // Move constructor
    bitVector(bitVector&& r) noexcept : _bitArray(nullptr), _size(0), _nchar(0) {
        *this = std::move(r);
    }

    void resize(uint64_t newsize) {
        _nchar = 1ULL + newsize / 64ULL;
        delete[] _bitArray;
        _bitArray = new std::atomic<uint64_t>[_nchar]();
        _size = newsize;
    }

    [[nodiscard]] size_t size() const noexcept { return _size; }

    [[nodiscard]] uint64_t bitSize() const noexcept { 
        return _nchar * 64ULL + _ranks.capacity() * 64ULL; 
    }

    /// Clear the entire bit array
    void clear() {
        for (size_t i = 0; i < _nchar; ++i) {
            _bitArray[i].store(0, std::memory_order_relaxed);
        }
    }

    /// Clear collisions in interval (start and size must be multiples of 64)
    void clearCollisions(uint64_t start, size_t size, bitVector* cc) {
        assert((start & 63) == 0);
        assert((size & 63) == 0);
        
        const uint64_t ids = start / 64ULL;
        for (uint64_t ii = 0; ii < (size / 64ULL); ++ii) {
            _bitArray[ids + ii] = _bitArray[ids + ii] & (~(cc->get64(ii)));
        }
        cc->clear();
    }

    /// Clear interval (start and size must be multiples of 64)
    void clear(uint64_t start, size_t size) {
        assert((start & 63) == 0);
        assert((size & 63) == 0);
        
        for (uint64_t ii = 0; ii < (size / 64ULL); ++ii) {
            _bitArray[(start / 64ULL) + ii] = 0;
        }
    }

    /// Print bit vector for debugging
    void print() const {
        std::cout << "bit array of size " << _size << " : " << std::endl;
        for (size_t ii = 0; ii < _size; ++ii) {
            if (ii % 10 == 0) {
                std::cout << " (" << ii << ") ";
            }
            const auto val = (_bitArray[ii >> 6] >> (ii & 63)) & 1;
            std::cout << val;
        }
        std::cout << std::endl;

        std::cout << "rank array : size " << _ranks.size() << std::endl;
        for (size_t ii = 0; ii < _ranks.size(); ++ii) {
            std::cout << ii << " :  " << _ranks[ii] << " , ";
        }
        std::cout << std::endl;
    }

    /// Get bit value at position
    [[nodiscard]] uint64_t operator[](uint64_t pos) const {
        return (_bitArray[pos >> 6].load(std::memory_order_relaxed) >> (pos & 63)) & 1;
    }

    /// Atomically test and set bit (returns old value)
    [[nodiscard]] inline uint64_t atomic_test_and_set(uint64_t pos) {
        const uint64_t mask = 1ULL << (pos & 63);
        auto* word = reinterpret_cast<std::atomic<uint64_t>*>(_bitArray + (pos >> 6));
        uint64_t oldval = word->load(std::memory_order_seq_cst);
        while (!word->compare_exchange_weak(oldval, oldval | mask, std::memory_order_seq_cst)) {
        }
        return (oldval >> (pos & 63)) & 1;
    }

    [[nodiscard]] uint64_t get(uint64_t pos) const { return (*this)[pos]; }

    [[nodiscard]] uint64_t get64(uint64_t cell64) const {
        return _bitArray[cell64].load(std::memory_order_relaxed);
    }

    /// Set bit at position to 1
    void set(uint64_t pos) {
        _bitArray[pos >> 6].fetch_or(1ULL << (pos & 63), std::memory_order_relaxed);
    }

    /// Set bit at position to 0
    void reset(uint64_t pos) {
        _bitArray[pos >> 6].fetch_and(~(1ULL << (pos & 63)), std::memory_order_relaxed);
    }

    /// Build rank structure, returns final rank value
    [[nodiscard]] uint64_t build_ranks(uint64_t offset = 0) {
        _ranks.reserve(2 + _size / NB_BITS_PER_RANK_SAMPLE);

        uint64_t current_rank = offset;
        for (size_t ii = 0; ii < _nchar; ++ii) {
            if ((ii * 64) % NB_BITS_PER_RANK_SAMPLE == 0) {
                _ranks.push_back(current_rank);
            }
            current_rank += popcount_64(_bitArray[ii].load(std::memory_order_relaxed));
        }
        return current_rank;
    }

    [[nodiscard]] uint64_t rank(uint64_t pos) const {
        const uint64_t word_idx = pos / 64ULL;
        const uint64_t word_offset = pos % 64;
        const uint64_t block = pos / NB_BITS_PER_RANK_SAMPLE;
        
        uint64_t r = _ranks[block];
        for (uint64_t w = block * NB_BITS_PER_RANK_SAMPLE / 64; w < word_idx; ++w) {
            r += popcount_64(_bitArray[w]);
        }
        
        const uint64_t mask = (uint64_t(1) << word_offset) - 1;
        r += popcount_64(_bitArray[word_idx] & mask);
        return r;
    }

    void save(std::ostream& os) const {
        boomphf::write_le(os, _size);
        boomphf::write_le(os, _nchar);
        
        // Save bit array - convert atomic to regular uint64_t
        std::vector<uint64_t> temp_array(_nchar);
        for (size_t i = 0; i < _nchar; ++i) {
            temp_array[i] = _bitArray[i].load(std::memory_order_relaxed);
        }
        boomphf::write_le_array(os, temp_array.data(), _nchar);
        
        const uint64_t sizer = _ranks.size();
        boomphf::write_le(os, sizer);
        boomphf::write_le_array(os, _ranks.data(), _ranks.size());
    }

    void load(std::istream& is) {
        boomphf::read_le(is, _size);
        boomphf::read_le(is, _nchar);
        resize(_size);
        
        // Load bit array - read into temp buffer then copy to atomic array
        std::vector<uint64_t> temp_array(_nchar);
        boomphf::read_le_array(is, temp_array.data(), _nchar);
        for (size_t i = 0; i < _nchar; ++i) {
            _bitArray[i].store(temp_array[i], std::memory_order_relaxed);
        }

        uint64_t sizer;
        boomphf::read_le(is, sizer);
        _ranks.resize(sizer);
        boomphf::read_le_array(is, _ranks.data(), _ranks.size());
    }

private:
    std::atomic<uint64_t>* _bitArray = nullptr;
    uint64_t _size = 0;
    uint64_t _nchar = 0;

    /// Rank sampling rate - balance between space and query time
    static constexpr uint64_t NB_BITS_PER_RANK_SAMPLE = 512;
    std::vector<uint64_t> _ranks;
};

} // namespace boomphf