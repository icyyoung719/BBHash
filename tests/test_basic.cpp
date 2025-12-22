#include "catch2/catch.hpp"
#include "BooPHF.h"
#include <vector>
#include <algorithm>

typedef boomphf::SingleHashFunctor<uint64_t> hasher_t;
typedef boomphf::mphf<uint64_t, hasher_t> boophf_t;

TEST_CASE("Basic MPHF construction and lookup", "[basic]") {
    // Create simple test data
    std::vector<uint64_t> data;
    for (uint64_t i = 0; i < 100; i++) {
        data.push_back(i);
    }
    
    // Build the MPHF
    boophf_t bphf(data.size(), data, 1, 1.0, false, false);
    
    SECTION("All keys can be looked up") {
        for (size_t i = 0; i < data.size(); i++) {
            uint64_t idx = bphf.lookup(data[i]);
            REQUIRE(idx < data.size());
        }
    }
    
    SECTION("Different keys get different indices") {
        std::vector<uint64_t> indices;
        for (const auto& key : data) {
            indices.push_back(bphf.lookup(key));
        }
        
        std::sort(indices.begin(), indices.end());
        auto last = std::unique(indices.begin(), indices.end());
        indices.erase(last, indices.end());
        
        // All indices should be unique (perfect hash)
        REQUIRE(indices.size() == data.size());
    }
}

TEST_CASE("MPHF with different gamma values", "[gamma]") {
    std::vector<uint64_t> data;
    for (uint64_t i = 0; i < 1000; i++) {
        data.push_back(i * 2);
    }
    
    SECTION("Gamma 1.0") {
        boophf_t bphf(data.size(), data, 1, 1.0, false, false);
        for (const auto& key : data) {
            REQUIRE(bphf.lookup(key) < data.size());
        }
    }
    
    SECTION("Gamma 2.0") {
        boophf_t bphf(data.size(), data, 1, 2.0, false, false);
        for (const auto& key : data) {
            REQUIRE(bphf.lookup(key) < data.size());
        }
    }
    
    SECTION("Gamma 3.0") {
        boophf_t bphf(data.size(), data, 1, 3.0, false, false);
        for (const auto& key : data) {
            REQUIRE(bphf.lookup(key) < data.size());
        }
    }
}

TEST_CASE("MPHF with different data sizes", "[sizes]") {
    SECTION("Small dataset (10 elements)") {
        std::vector<uint64_t> data;
        for (uint64_t i = 0; i < 10; i++) {
            data.push_back(i);
        }
        
        boophf_t bphf(data.size(), data, 1, 1.0, false, false);
        for (const auto& key : data) {
            REQUIRE(bphf.lookup(key) < data.size());
        }
    }
    
    SECTION("Medium dataset (1000 elements)") {
        std::vector<uint64_t> data;
        for (uint64_t i = 0; i < 1000; i++) {
            data.push_back(i);
        }
        
        boophf_t bphf(data.size(), data, 1, 1.0, false, false);
        for (const auto& key : data) {
            REQUIRE(bphf.lookup(key) < data.size());
        }
    }
    
    SECTION("Large dataset (10000 elements)") {
        std::vector<uint64_t> data;
        for (uint64_t i = 0; i < 10000; i++) {
            data.push_back(i);
        }
        
        boophf_t bphf(data.size(), data, 1, 1.0, false, false);
        for (const auto& key : data) {
            REQUIRE(bphf.lookup(key) < data.size());
        }
    }
}
