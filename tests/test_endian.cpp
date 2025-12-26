#include "BooPHF.h"
#include "catch2/catch.hpp"
#include <cstdio>
#include <fstream>
#include <vector>

typedef boomphf::SingleHashFunctor<uint64_t> hasher_t;
typedef boomphf::mphf<uint64_t, hasher_t> boophf_t;

// Helper function to test endianness-safe serialization
void test_endian_serialization(size_t num_keys, double gamma, const std::string& test_name)
{
	// Create test data
	std::vector<uint64_t> data;
	for (uint64_t i = 0; i < num_keys; i++)
	{
		data.push_back(i * 2); // Simple predictable data
	}

	// Build the MPHF
	boophf_t bphf(data.size(), data, 1, gamma, false, false);

	// Save to file
	std::string filename = "test_endian_" + test_name + ".mphf";
	{
		std::ofstream os(filename, std::ios::binary);
		REQUIRE(os.is_open());
		bphf.save(os);
		os.close();
	}

	// Load from file
	boophf_t bphf_load;
	{
		std::ifstream is(filename, std::ios::binary);
		REQUIRE(is.is_open());
		bphf_load.load(is);
		is.close();
	}

	// Verify all queries match
	for (size_t i = 0; i < data.size(); i++)
	{
		uint64_t idx_orig = bphf.lookup(data[i]);
		uint64_t idx_load = bphf_load.lookup(data[i]);

		REQUIRE(idx_orig == idx_load);
		REQUIRE(idx_load < data.size());
	}

	// Clean up
	std::remove(filename.c_str());
}

TEST_CASE("Endianness-safe serialization with small dataset", "[endian][small]")
{
	test_endian_serialization(100, 1.0, "small_gamma1");
}

TEST_CASE("Endianness-safe serialization with medium dataset", "[endian][medium]")
{
	SECTION("Gamma 1.0") { test_endian_serialization(1000, 1.0, "medium_gamma1"); }

	SECTION("Gamma 2.0") { test_endian_serialization(1000, 2.0, "medium_gamma2"); }

	SECTION("Gamma 3.0") { test_endian_serialization(1000, 3.0, "medium_gamma3"); }
}

TEST_CASE("Endianness-safe serialization with large dataset", "[endian][large]")
{
	test_endian_serialization(10000, 1.0, "large_gamma1");
}
