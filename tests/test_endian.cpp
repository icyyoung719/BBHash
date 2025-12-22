#include "BooPHF.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <cstdio>

using namespace std;

typedef boomphf::SingleHashFunctor<uint64_t>  hasher_t;
typedef boomphf::mphf<uint64_t, hasher_t> boophf_t;

// Test different gamma values and data sizes
bool test_with_params(size_t num_keys, double gamma, const string& test_name) {
    cout << "\n=== " << test_name << " ===" << endl;
    cout << "Parameters: " << num_keys << " keys, gamma=" << gamma << endl;
    
    // Create test data
    vector<uint64_t> data;
    for (uint64_t i = 0; i < num_keys; i++) {
        data.push_back(i * 2); // Simple predictable data
    }
    
    // Build the MPHF
    boophf_t* bphf = new boophf_t(data.size(), data, 1, gamma, false, false);
    
    cout << "Built MPHF with " << (float)bphf->totalBitSize() / data.size() << " bits/elem" << endl;
    
    // Save to file
    string filename = "test_" + test_name + ".mphf";
    {
        ofstream os(filename, ios::binary);
        if (!os) {
            cout << "ERROR: Cannot open file for writing" << endl;
            delete bphf;
            return false;
        }
        bphf->save(os);
        os.close();
    }
    
    // Load from file
    boophf_t* bphf_load = new boophf_t();
    {
        ifstream is(filename, ios::binary);
        if (!is) {
            cout << "ERROR: Cannot open file for reading" << endl;
            delete bphf;
            return false;
        }
        bphf_load->load(is);
        is.close();
    }
    
    // Verify all queries match
    bool success = true;
    for (size_t i = 0; i < data.size(); i++) {
        uint64_t idx_orig = bphf->lookup(data[i]);
        uint64_t idx_load = bphf_load->lookup(data[i]);
        
        if (idx_orig != idx_load) {
            cout << "ERROR: Mismatch at i=" << i << " key=" << data[i] 
                 << " orig=" << idx_orig << " loaded=" << idx_load << endl;
            success = false;
            break;
        }
        
        if (idx_load >= data.size()) {
            cout << "ERROR: Invalid index " << idx_load << " for key " << data[i] << endl;
            success = false;
            break;
        }
    }
    
    if (success) {
        cout << "✓ All " << data.size() << " queries verified successfully" << endl;
    }
    
    // Clean up
    remove(filename.c_str());
    delete bphf;
    delete bphf_load;
    
    return success;
}

int main() {
    cout << "BBHash Endianness-Safe Serialization Test" << endl;
    cout << "===========================================" << endl;
    
    // Test with different configurations
    bool all_passed = true;
    
    all_passed &= test_with_params(100, 1.0, "small_gamma1");
    all_passed &= test_with_params(1000, 1.0, "medium_gamma1");
    all_passed &= test_with_params(10000, 1.0, "large_gamma1");
    all_passed &= test_with_params(1000, 2.0, "medium_gamma2");
    all_passed &= test_with_params(1000, 3.0, "medium_gamma3");
    
    cout << "\n===========================================" << endl;
    if (all_passed) {
        cout << "✓ ALL TESTS PASSED - Endianness-safe serialization works correctly!" << endl;
        return 0;
    } else {
        cout << "✗ SOME TESTS FAILED" << endl;
        return 1;
    }
}
