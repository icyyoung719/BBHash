#include "catch2/catch.hpp"
#include "BooPHF.h"
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <thread>
#include <iterator>
#include <random>
#include <memory>
#include <map>
#include <set>
#include <sstream>

typedef boomphf::SingleHashFunctor<uint64_t> hasher_t;
typedef boomphf::mphf<uint64_t, hasher_t> boophf_t;

TEST_CASE("Multi-thread builds equal across thread counts (binary + lookup)", "[multithread]") {
	// Prepare deterministic test data
	const size_t N = 20000;
	std::vector<uint64_t> data;
	data.reserve(N);
	std::mt19937_64 rng(42); // deterministic seed for reproducibility
	std::uniform_int_distribution<uint64_t> dist;
	for (size_t i = 0; i < N; ++i) {
		data.push_back(dist(rng));
	}

	// Candidate thread counts to test (will be clipped to hardware concurrency)
	std::vector<uint32_t> candidate_counts = {1u, 2u, 4u, 8u};
	const uint32_t hw = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1u;

	// Normalize counts: keep unique, at least 1, and not exceed hw
	std::set<uint32_t> counts_set;
	for (auto c : candidate_counts) {
		if (c == 0) continue;
		counts_set.insert(std::min(c, hw));
	}
	counts_set.insert(1u);

	auto set_to_string = [](const std::set<uint32_t>& s) {
		std::ostringstream oss;
		oss << "{ ";
		for (auto v : s) {
			oss << v << " ";
		}
		oss << "}";
		return oss.str();
	};
	std::cout << "Testing thread counts: " << set_to_string(counts_set) << std::endl;
	

	// Prepare containers for built MPHFs and saved file data
	std::map<uint32_t, std::unique_ptr<boophf_t>> built;
	std::map<uint32_t, std::vector<char>> file_bytes;

	for (auto c : counts_set) {
		// Build
		auto phf = std::make_unique<boophf_t>(data.size(), data, static_cast<int>(c), 1.0, false, false);

		// Save to file
		std::ostringstream fname;
		fname << "test_threads_" << c << ".mphf";
		std::ofstream os(fname.str(), std::ios::binary);
		REQUIRE(os.is_open());
		phf->save(os);
		os.close();

		// Read file bytes
		std::ifstream is(fname.str(), std::ios::binary);
		REQUIRE(is.is_open());
		std::vector<char> buf((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
		is.close();

		file_bytes[c] = std::move(buf);
		built[c] = std::move(phf);
	}

	// Choose baseline (1 thread)
	const uint32_t baseline = 1u;
	REQUIRE(file_bytes.find(baseline) != file_bytes.end());

	// Compare binary outputs and lookups
	for (const auto &kv : file_bytes) {
		const uint32_t c = kv.first;
		const auto &buf = kv.second;
		// Binary equality
		REQUIRE(buf.size() == file_bytes[baseline].size());
		REQUIRE(buf == file_bytes[baseline]);

		// Lookup equality
		for (const auto &k : data) {
			const auto base_idx = built[baseline]->lookup(k);
			const auto idx = built[c]->lookup(k);
			REQUIRE(base_idx == idx);
			REQUIRE(base_idx < data.size());
		}
	}

	// Clean up files
	for (const auto &kv : file_bytes) {
		std::ostringstream fname;
		fname << "test_threads_" << kv.first << ".mphf";
		std::remove(fname.str().c_str());
	}
}