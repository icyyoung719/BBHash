#include "BooPHF.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>

using namespace std;

typedef boomphf::SingleHashFunctor<uint64_t>  hasher_t;
typedef boomphf::mphf<uint64_t, hasher_t> boophf_t;

int main() {
    // Create a small set of test data
    vector<uint64_t> data;
    for (uint64_t i = 0; i < 1000; i++) {
        data.push_back(i * 2); // Simple predictable data
    }
    
    cout << "Building MPHF with " << data.size() << " elements..." << endl;
    
    // Build the MPHF
    boophf_t* bphf = new boophf_t(data.size(), data, 1, 1.0, false, false);
    
    cout << "MPHF built successfully" << endl;
    cout << "Bits per element: " << (float)bphf->totalBitSize() / data.size() << endl;
    
    // Test queries before save/load
    cout << "Testing queries before save..." << endl;
    for (size_t i = 0; i < min(size_t(10), data.size()); i++) {
        uint64_t idx = bphf->lookup(data[i]);
        if (idx >= data.size()) {
            cout << "ERROR: Invalid index " << idx << " for key " << data[i] << endl;
            return 1;
        }
    }
    cout << "Queries before save: OK" << endl;
    
    // Save to file
    cout << "Saving to file..." << endl;
    {
        ofstream os("test_endian.mphf", ios::binary);
        if (!os) {
            cout << "ERROR: Cannot open file for writing" << endl;
            return 1;
        }
        bphf->save(os);
        os.close();
    }
    cout << "Saved successfully" << endl;
    
    // Load from file
    cout << "Loading from file..." << endl;
    boophf_t* bphf_load = new boophf_t();
    {
        ifstream is("test_endian.mphf", ios::binary);
        if (!is) {
            cout << "ERROR: Cannot open file for reading" << endl;
            return 1;
        }
        bphf_load->load(is);
        is.close();
    }
    cout << "Loaded successfully" << endl;
    
    // Test queries after load
    cout << "Testing queries after load..." << endl;
    for (size_t i = 0; i < data.size(); i++) {
        uint64_t idx_orig = bphf->lookup(data[i]);
        uint64_t idx_load = bphf_load->lookup(data[i]);
        
        if (idx_orig != idx_load) {
            cout << "ERROR: Mismatch at i=" << i << " key=" << data[i] 
                 << " orig=" << idx_orig << " loaded=" << idx_load << endl;
            return 1;
        }
        
        if (idx_load >= data.size()) {
            cout << "ERROR: Invalid index " << idx_load << " for key " << data[i] << endl;
            return 1;
        }
    }
    cout << "All queries after load: OK" << endl;
    
    delete bphf;
    delete bphf_load;
    
    cout << "SUCCESS: All tests passed!" << endl;
    return 0;
}
