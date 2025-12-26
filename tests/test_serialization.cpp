#include "BooPHF.h"
#include "catch2/catch.hpp"
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

typedef boomphf::SingleHashFunctor<uint64_t> hasher_t;
typedef boomphf::mphf<uint64_t, hasher_t> boophf_t;

TEST_CASE("MPHF serialization and deserialization", "[serialization]")
{
	std::vector<uint64_t> data;
	for (uint64_t i = 0; i < 1000; i++)
	{
		data.push_back(i * 3);
	}

	// Build the MPHF
	auto bphf = std::make_unique<boophf_t>(data.size(), data, 1, 1.0, false, false);

	SECTION("Save and load from file")
	{
		const char* filename = "test_serialization.mphf";

		// Save to file
		{
			std::ofstream os(filename, std::ios::binary);
			REQUIRE(os.is_open());
			bphf->save(os);
			os.close();
		}

		// Load from file
		auto bphf_load = std::make_unique<boophf_t>();
		{
			std::ifstream is(filename, std::ios::binary);
			REQUIRE(is.is_open());
			bphf_load->load(is);
			is.close();
		}

		// Verify all queries match
		for (size_t i = 0; i < data.size(); i++)
		{
			uint64_t idx_orig = bphf->lookup(data[i]);
			uint64_t idx_load = bphf_load->lookup(data[i]);

			REQUIRE(idx_orig == idx_load);
			REQUIRE(idx_load < data.size());
		}

		// Clean up
		std::remove(filename);
	}
}

TEST_CASE("MPHF serialization with different gamma values", "[serialization][gamma]")
{
	std::vector<uint64_t> data;
	for (uint64_t i = 0; i < 500; i++)
	{
		data.push_back(i * 7);
	}

	SECTION("Gamma 2.0")
	{
		const char* filename = "test_gamma2.mphf";

		auto bphf = std::make_unique<boophf_t>(data.size(), data, 1, 2.0, false, false);

		{
			std::ofstream os(filename, std::ios::binary);
			bphf->save(os);
		}

		auto bphf_load = std::make_unique<boophf_t>();
		{
			std::ifstream is(filename, std::ios::binary);
			bphf_load->load(is);
		}

		for (const auto& key : data)
		{
			REQUIRE(bphf->lookup(key) == bphf_load->lookup(key));
		}

		std::remove(filename);
	}
}
