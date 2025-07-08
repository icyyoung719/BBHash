#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cassert>
// #include "bbhash/MurmurHash3.h"
// #include "bbhash/bbhash.hpp"
#include "BooPHF.h"

// 编码 pair<int,int> 为 uint64_t
inline uint64_t encode_pair(int a, int b) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(a)) << 32) |
           static_cast<uint32_t>(b);
}

int main() {
    // 生成模拟数据：500万个pair和float值
    size_t N = 5000000;

    std::vector<uint64_t> keys;
    keys.reserve(N);
    std::vector<float> values;
    values.reserve(N);

    // 随机数生成器
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist_int(0, 100000000);
    std::uniform_real_distribution<float> dist_float(0.0f, 1.0f);

    for (size_t i = 0; i < N; ++i) {
        int a = dist_int(rng);
        int b = dist_int(rng);
        keys.push_back(encode_pair(a, b));
        values.push_back(dist_float(rng));
    }

    // 构造 BBHash 最小完美哈希
    double gamma = 2.0; // 构建时的空间/速度权衡参数，2.0较安全
    boomphf::mphf<uint64_t> mphf(keys.size(), keys.data(), keys.size(), gamma);

    std::cout << "MPHF built for " << keys.size() << " keys." << std::endl;

    // 通过 mphf 查询演示
    // 选取一个已知 key 测试查询正确性
    size_t test_idx = N / 2;
    uint64_t test_key = keys[test_idx];
    size_t pos = mphf.lookup(test_key);
    assert(pos < values.size());
    std::cout << "Lookup test key at index " << test_idx << ": value = " << values[pos] << std::endl;

    // 测试一个不存在的key（结果未定义，但一般返回范围内）
    uint64_t missing_key = encode_pair(-1, -1);
    size_t missing_pos = mphf.lookup(missing_key);
    std::cout << "Lookup missing key, index returned: " << missing_pos << std::endl;

    return 0;
}
