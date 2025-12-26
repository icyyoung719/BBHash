
#include "BooPHF.h"
#include "catch2/catch.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

static std::filesystem::path find_build_dir()
{
	auto p = std::filesystem::current_path();
	for (;;)
	{
		auto candidate = p / "build";
		if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate))
			return candidate;
		if (!p.has_parent_path())
			break;
		p = p.parent_path();
	}
	// fallback: try sibling "build" relative to source tree
	return std::filesystem::current_path() / "build";
}

TEST_CASE("random_generated_keys_min", "[min]")
{
	// Randomly generated data for testing with fixed seed for reproducibility
	// format: prev,next,value (value not used)
	std::unordered_set<uint64_t> key_set; // deduplicated keys
	std::mt19937 gen(42);
	std::uniform_int_distribution<int32_t> dist(INT32_MIN, INT32_MAX);

	std::cout << "Generating random keys..." << std::endl;
	int max_num = 1000000; // generated count
	for (int i = 0; i < max_num; ++i)
	{
		int32_t prev = dist(gen);
		int32_t next = dist(gen);
		uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(prev)) << 32) | static_cast<uint32_t>(next);
		key_set.insert(key);
	}

	std::vector<uint64_t> input_keys(key_set.begin(), key_set.end());
	std::sort(input_keys.begin(), input_keys.end());

	// Save generated keys to CSV under the repository build directory for debugging
	std::cout << "Saving generated keys to CSV..." << std::endl;
	auto build_dir = find_build_dir();
	std::error_code ec;
	if (!std::filesystem::exists(build_dir))
		std::filesystem::create_directories(build_dir, ec);
	auto csv_path = build_dir / "random_generated.csv";
	std::ofstream csv(csv_path);
	REQUIRE(csv.is_open());
	csv << "prev,next,combined\n";
	for (auto k : input_keys)
	{
		uint32_t prev = static_cast<uint32_t>(k >> 32);
		uint32_t next = static_cast<uint32_t>(k & 0xFFFFFFFFu);
		csv << static_cast<int32_t>(prev) << ',' << static_cast<int32_t>(next) << ',' << k << '\n';
	}
	csv.flush();
	csv.close();

	// Build BooPHF
	std::cout << "Building BooPHF..." << std::endl;
	using boophf_t = boomphf::mphf<uint64_t, boomphf::SingleHashFunctor<uint64_t>>;
	boophf_t bphf(input_keys.size(), input_keys, 1, 1, false);

	std::ofstream os("example.mphf", std::ios::binary);
	REQUIRE(os.is_open());
	bphf.save(os);
	os.close();

	boophf_t bphf_load;
	std::ifstream is("example.mphf", std::ios::binary);
	REQUIRE(is.is_open());
	bphf_load.load(is);
	is.close();

	// Test queries using Catch so checks remain in Release builds
	std::cout << "Testing queries..." << std::endl;
	for (const auto& key : input_keys)
	{
		uint64_t idx = bphf_load.lookup(key);
		REQUIRE(idx < input_keys.size());
		REQUIRE(idx == bphf.lookup(key));
	}
}
