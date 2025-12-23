#include "BooPHF.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

int main() {
    // Simple example demonstrating BBHash usage
    
    // 1. Generate some random keys
    const size_t num_keys = 100000;
    std::vector<uint64_t> keys;
    keys.reserve(num_keys);
    
    std::mt19937_64 rng(42);  // Fixed seed for reproducible, deterministic output
    for (size_t i = 0; i < num_keys; ++i) {
        keys.push_back(rng());
    }
    
    std::cout << "Generated " << num_keys << " random keys" << std::endl;
    
    // 2. Build the Minimal Perfect Hash Function (MPHF)
    typedef boomphf::SingleHashFunctor<uint64_t> hasher_t;
    typedef boomphf::mphf<uint64_t, hasher_t> boophf_t;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    double gamma = 2.0;  // Space/time trade-off parameter (1.0-3.0 typical range)
    int num_threads = 1; // Number of threads to use
    boophf_t mphf(keys.size(), keys, num_threads, gamma, false, false);
    
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    
    std::cout << "Built MPHF in " << elapsed << " seconds" << std::endl;
    std::cout << "Space usage: " << (float)mphf.totalBitSize() / num_keys << " bits/key" << std::endl;
    
    // 3. Test lookups
    std::cout << "\nTesting lookups:" << std::endl;
    for (size_t i = 0; i < 5; ++i) {
        uint64_t key = keys[i];
        uint64_t hash = mphf.lookup(key);
        std::cout << "  Key " << key << " -> hash " << hash << std::endl;
    }
    
    // 4. Verify that all hashes are unique and in range [0, num_keys)
    std::vector<bool> seen(num_keys, false);
    bool all_unique = true;
    
    for (const auto& key : keys) {
        uint64_t hash = mphf.lookup(key);
        if (hash >= num_keys || seen[hash]) {
            all_unique = false;
            break;
        }
        seen[hash] = true;
    }
    
    std::cout << "\nVerification: " << (all_unique ? "PASSED" : "FAILED") << std::endl;
    std::cout << "All keys map to unique values in [0, " << num_keys << ")" << std::endl;
    
    return 0;
}
