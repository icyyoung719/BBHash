#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <cstdint>
#include "BooPHF.h"

int main() {
    std::ifstream infile("C:\\Users\\Administrator\\Desktop\\BBHash\\BBHash\\converted_m2.csv");
    if (!infile) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    std::unordered_set<uint64_t> key_set; // 去重
    std::string line;
    std::getline(infile, line); // 跳过表头

    int max_num = 1000000; // 限制读取的行数
    int counter = 0;
    while (std::getline(infile, line)) {
        if (counter >= max_num) break;
        counter++;
        std::istringstream iss(line);
        std::string prev_str, next_str, val_str;

        if (!std::getline(iss, prev_str, ',')) continue;
        if (!std::getline(iss, next_str, ',')) continue;
        // 忽略 value
        if (!std::getline(iss, val_str, ',')) continue;

        int32_t prev = std::stoi(prev_str);
        int32_t next = std::stoi(next_str);

        uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(prev)) << 32) |
                        static_cast<uint32_t>(next);

        key_set.insert(key); // 去重插入
    }

    std::vector<uint64_t> input_keys(key_set.begin(), key_set.end());

    // 构建 BooPHF
    using boophf_t = boomphf::mphf<uint64_t, boomphf::SingleHashFunctor<uint64_t>>;
    boophf_t bphf(input_keys.size(), input_keys, 1, 1, false);

    // 查询每个 key 的索引
    int i = 0;
    std::string queryOut = R"(C:\Users\Administrator\Desktop\BBHash\BBHash\query.csv)";
    std::ofstream queryFile(queryOut);

    for (const auto& key : input_keys) {
        i++;
        if (i >= 100)
            break;
        uint64_t idx = bphf.lookup(key);
        queryFile << idx << "," << key << "\n";
        // std::cout << "Key: " << key << " -> Index: " << idx << "\n";
        // assert (idx < input_keys.size());
        assert (idx < input_keys.size());
    }

	// std::ofstream os("example.mphf", std::ios::binary);
	// bphf.save(os);

    // os.flush();

	boophf_t* bphf_load = new boophf_t;
	std::ifstream is("example.mphf", std::ios::binary);
	bphf_load->load(is);

    // TEST query
    for (const auto& key : input_keys) {
        uint64_t idx = bphf_load->lookup(key);
        // std::cout << "Key: " << key << " -> Index: " << idx << "\n";
        assert (idx < input_keys.size());
        assert(idx == bphf.lookup(key));
    }

    std::cout << "All tests passed.\n";

    return 0;
}
