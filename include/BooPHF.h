/// BooPHF - Minimal perfect hash function library
/// Fast and low memory construction, with slightly higher bits/elem than other libraries.
/// Supports arbitrarily large number of elements via cascade of collision-free bit arrays.

#pragma once

#include <array>
#include <cassert>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "bitvector.hpp"
#include "endian_utils.hpp"
#include "platform_time.h"
#include "progress.hpp"

namespace boomphf
{

/// Buffered file iterator for reading binary data
template <typename basetype>
class bfile_iterator
{
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = basetype;
	using difference_type = std::ptrdiff_t;
	using pointer = const basetype*;
	using reference = const basetype&;

	bfile_iterator() : _is(nullptr), _pos(0), _inbuff(0), _cptread(0), _buffsize(10000)
	{
		_buffer = static_cast<basetype*>(std::malloc(_buffsize * sizeof(basetype)));
	}

	bfile_iterator(const bfile_iterator& cr)
	    : _is(cr._is), _pos(cr._pos), _buffsize(cr._buffsize),
	      _inbuff(cr._inbuff), _cptread(cr._cptread), _elem(cr._elem)
	{
		_buffer = static_cast<basetype*>(std::malloc(_buffsize * sizeof(basetype)));
		std::memcpy(_buffer, cr._buffer, _buffsize * sizeof(basetype));
	}

	explicit bfile_iterator(FILE* is)
	    : _is(is), _pos(0), _inbuff(0), _cptread(0), _buffsize(10000)
	{
		_buffer = static_cast<basetype*>(std::malloc(_buffsize * sizeof(basetype)));
		std::fseek(_is, 0, SEEK_SET);
		advance();
	}

	~bfile_iterator()
	{
		std::free(_buffer);
	}

	reference operator*() const { return _elem; }

	bfile_iterator& operator++()
	{
		advance();
		return *this;
	}

	friend bool operator==(const bfile_iterator& lhs, const bfile_iterator& rhs)
	{
		if (!lhs._is || !rhs._is)
		{
			return !lhs._is && !rhs._is;
		}
		assert(lhs._is == rhs._is);
		return rhs._pos == lhs._pos;
	}

	friend bool operator!=(const bfile_iterator& lhs, const bfile_iterator& rhs)
	{
		return !(lhs == rhs);
	}

private:
	void advance()
	{
		++_pos;

		if (_cptread >= _inbuff)
		{
			const size_t res = std::fread(_buffer, sizeof(basetype), _buffsize, _is);
			_inbuff = res;
			_cptread = 0;

			if (res == 0)
			{
				_is = nullptr;
				_pos = 0;
				return;
			}
		}

		_elem = _buffer[_cptread];
		++_cptread;
	}

	basetype _elem;
	FILE* _is;
	uint64_t _pos;
	basetype* _buffer;
	size_t _inbuff;
	size_t _cptread;
	size_t _buffsize;
};

/// Binary file reader with iterator interface
template <typename type_elem>
class file_binary
{
public:
	explicit file_binary(const char* filename)
	{
		_is = std::fopen(filename, "rb");
		if (!_is)
		{
			throw std::invalid_argument("Error opening " + std::string(filename));
		}
	}

	explicit file_binary(const std::string& filename) : file_binary(filename.c_str()) {}

	~file_binary()
	{
		std::fclose(_is);
	}

	[[nodiscard]] bfile_iterator<type_elem> begin() const
	{
		return bfile_iterator<type_elem>(_is);
	}

	[[nodiscard]] bfile_iterator<type_elem> end() const
	{
		return bfile_iterator<type_elem>();
	}

	[[nodiscard]] size_t size() const { return 0; }

private:
	FILE* _is;
};

////////////////////////////////////////////////////////////////
// Hash functions
////////////////////////////////////////////////////////////////

using hash_set_t = std::array<uint64_t, 10>;
using hash_pair_t = std::array<uint64_t, 2>;

/// Hash function generator for Items
template <typename Item>
class HashFunctors
{
public:
	HashFunctors() : _nbFct(7), _user_seed(0)
	{
		generate_hash_seed();
	}

	[[nodiscard]] uint64_t operator()(const Item& key, size_t idx) const
	{
		return hash64(key, _seed_tab[idx]);
	}

	[[nodiscard]] uint64_t hashWithSeed(const Item& key, uint64_t seed) const
	{
		return hash64(key, seed);
	}

	/// Returns all hash values for the key
	[[nodiscard]] hash_set_t operator()(const Item& key) const
	{
		hash_set_t hset;
		for (size_t ii = 0; ii < 10; ++ii)
		{
			hset[ii] = hash64(key, _seed_tab[ii]);
		}
		return hset;
	}

private:
	[[nodiscard]] static inline uint64_t hash64(Item key, uint64_t seed)
	{
		uint64_t hash = seed;
		hash ^= (hash << 7) ^ key * (hash >> 3) ^ (~((hash << 11) + (key ^ (hash >> 5))));
		hash = (~hash) + (hash << 21);
		hash = hash ^ (hash >> 24);
		hash = (hash + (hash << 3)) + (hash << 8);
		hash = hash ^ (hash >> 14);
		hash = (hash + (hash << 2)) + (hash << 4);
		hash = hash ^ (hash >> 28);
		hash = hash + (hash << 31);
		return hash;
	}

	void generate_hash_seed()
	{
		static constexpr uint64_t rbase[MAXNBFUNC] = {
		    0xAAAAAAAA55555555ULL, 0x33333333CCCCCCCCULL, 0x6666666699999999ULL, 0xB5B5B5B54B4B4B4BULL,
		    0xAA55AA5555335533ULL, 0x33CC33CCCC66CC66ULL, 0x6699669999B599B5ULL, 0xB54BB54B4BAA4BAAULL,
		    0xAA33AA3355CC55CCULL, 0x33663366CC99CC99ULL};

		for (size_t i = 0; i < MAXNBFUNC; ++i)
		{
			_seed_tab[i] = rbase[i];
		}
		for (size_t i = 0; i < MAXNBFUNC; ++i)
		{
			_seed_tab[i] = _seed_tab[i] * _seed_tab[(i + 3) % MAXNBFUNC] + _user_seed;
		}
	}

	size_t _nbFct;
	static constexpr size_t MAXNBFUNC = 10;
	uint64_t _seed_tab[MAXNBFUNC];
	uint64_t _user_seed;
};

/// Wrapper to return single hash value instead of multiple hashes
template <typename Item>
class SingleHashFunctor
{
public:
	[[nodiscard]] uint64_t operator()(const Item& key, uint64_t seed = 0xAAAAAAAA55555555ULL) const
	{
		return hashFunctors.hashWithSeed(key, seed);
	}

private:
	HashFunctors<Item> hashFunctors;
};

/// Xorshift-based hash generator using a single hash functor
/// Based on Xorshift128* by Sebastiano Vigna (public domain)
template <typename Item, class SingleHasher_t>
class XorshiftHashFunctors
{
	static_assert(
	    std::is_invocable_r_v<uint64_t, const SingleHasher_t&, const Item&, uint64_t>,
	    "SingleHasher_t::operator()(const Item&, uint64_t) must be const and return uint64_t");

	[[nodiscard]] uint64_t next(uint64_t* s) const
	{
		uint64_t s1 = s[0];
		const uint64_t s0 = s[1];
		s[0] = s0;
		s1 ^= s1 << 23;
		return (s[1] = (s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26))) + s0;
	}

public:
	[[nodiscard]] uint64_t h0(hash_pair_t& s, const Item& key) const
	{
		s[0] = singleHasher(key, 0xAAAAAAAA55555555ULL);
		return s[0];
	}

	[[nodiscard]] uint64_t h1(hash_pair_t& s, const Item& key) const
	{
		s[1] = singleHasher(key, 0x33333333CCCCCCCCULL);
		return s[1];
	}

	[[nodiscard]] uint64_t next(hash_pair_t& s) const
	{
		uint64_t s1 = s[0];
		const uint64_t s0 = s[1];
		s[0] = s0;
		s1 ^= s1 << 23;
		return (s[1] = (s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26))) + s0;
	}

	[[nodiscard]] hash_set_t operator()(const Item& key) const
	{
		uint64_t s[2];
		hash_set_t hset;

		hset[0] = singleHasher(key, 0xAAAAAAAA55555555ULL);
		hset[1] = singleHasher(key, 0x33333333CCCCCCCCULL);
		s[0] = hset[0];
		s[1] = hset[1];

		for (size_t ii = 2; ii < 10; ++ii)
		{
			hset[ii] = next(s);
		}
		return hset;
	}

private:
	SingleHasher_t singleHasher;
};

////////////////////////////////////////////////////////////////
// Iterators
////////////////////////////////////////////////////////////////

/// Range wrapper for iterators
template <typename Iterator>
struct iter_range
{
	iter_range(Iterator b, Iterator e) : m_begin(b), m_end(e) {}

	[[nodiscard]] Iterator begin() const { return m_begin; }
	[[nodiscard]] Iterator end() const { return m_end; }

	Iterator m_begin, m_end;
};

template <typename Iterator>
[[nodiscard]] iter_range<Iterator> range(Iterator begin, Iterator end)
{
	return iter_range<Iterator>(begin, end);
}

////////////////////////////////////////////////////////////////
// Level structure
////////////////////////////////////////////////////////////////

[[nodiscard]] inline uint64_t fastrange64(uint64_t word, uint64_t p)
{
	return word % p;
}

class level
{
public:
	level() = default;
	~level() = default;

	[[nodiscard]] uint64_t get(uint64_t hash_raw) const
	{
		const uint64_t hashi = fastrange64(hash_raw, hash_domain);
		return bitset.get(hashi);
	}

	uint64_t idx_begin{0};
	uint64_t hash_domain{0};
	bitVector bitset;
};

////////////////////////////////////////////////////////////////
// Threading
////////////////////////////////////////////////////////////////

constexpr int NBBUFF = 10000;

template <typename Range, typename Iterator>
struct thread_args
{
	void* boophf;
	const Range* range;
	std::shared_ptr<void> it_p;
	std::shared_ptr<void> until_p;
	int level;
};

// Forward declaration
template <typename elem_t, typename Hasher_t, typename Range, typename it_type>
void thread_processLevel(thread_args<Range, it_type>* targ);

/// Minimal perfect hash function
/// Hasher_t returns a single hash when operator()(elem_t key) is called
template <typename elem_t, typename Hasher_t>
class mphf
{
	using MultiHasher_t = XorshiftHashFunctors<elem_t, Hasher_t>;

public:
	mphf() : _built(false) {}
	~mphf() = default;

	/// Construct MPHF from input range
	template <typename Range>
	mphf(uint64_t n, const Range& input_range, int num_thread = 1, double gamma = 2.0,
	     bool writeEach = true, bool progress = true, float perc_elem_loaded = 0.03)
	    : _gamma(gamma),
	      _hash_domain(static_cast<uint64_t>(std::ceil(static_cast<double>(n) * gamma))),
	      _nelem(n),
	      _num_thread(num_thread),
	      _percent_elem_loaded_for_fastMode(perc_elem_loaded),
	      _withprogress(progress)
	{

		if (n == 0)
		{
			return;
		}

		_fastmode = (_percent_elem_loaded_for_fastMode > 0.0);
		_writeEachLevel = writeEach;

		if (writeEach)
		{
			_fastmode = false;
		}

		setup();

		if (_withprogress)
		{
			_progressBar.timer_mode = 1;

			const double total_raw = _nb_levels;
			const double sum_geom_read = 1.0 / (1.0 - _proba_collision);
			const double total_writeEach = sum_geom_read + 1.0;
			const double total_fastmode_ram = (_fastModeLevel + 1) +
			                                  (std::pow(_proba_collision, _fastModeLevel)) * (_nb_levels - (_fastModeLevel + 1));

			std::printf("for info, total work write each  : %.3f    total work inram from level %i : %.3f  total work raw : %.3f \n",
			            total_writeEach, _fastModeLevel, total_fastmode_ram, total_raw);

			if (writeEach)
			{
				_progressBar.init(_nelem * static_cast<uint64_t>(total_writeEach), "Building BooPHF", num_thread);
			}
			else if (_fastmode)
			{
				_progressBar.init(_nelem * static_cast<uint64_t>(total_fastmode_ram), "Building BooPHF", num_thread);
			}
			else
			{
				_progressBar.init(_nelem * _nb_levels, "Building BooPHF", num_thread);
			}
		}

		uint64_t offset = 0;
		for (uint32_t ii = 0; ii < _nb_levels; ++ii)
		{
			_tempBitset = new bitVector(_levels[ii].hash_domain);
			processLevel(input_range, ii);
			_levels[ii].bitset.clearCollisions(0, _levels[ii].hash_domain, _tempBitset);
			offset = _levels[ii].bitset.build_ranks(offset);
			delete _tempBitset;
		}

		if (_withprogress)
		{
			_progressBar.finish_threaded();
		}

		_lastbitsetrank = offset;
		std::vector<elem_t>().swap(setLevelFastmode);

		std::lock_guard<std::mutex> lock(_mutex);
		_built = true;
	}

	/// Lookup hash value for an element
	[[nodiscard]] uint64_t lookup(const elem_t& elem) const
	{
		if (!_built)
		{
			return ULLONG_MAX;
		}

		hash_pair_t bbhash;
		int level;
		const uint64_t level_hash = getLevel(bbhash, elem, &level);

		if (level == static_cast<int>(_nb_levels) - 1)
		{
			const auto in_final_map = _final_hash.find(elem);
			if (in_final_map == _final_hash.end())
			{
				return ULLONG_MAX; // Element not in original set
			}
			return in_final_map->second + _lastbitsetrank;
		}

		const uint64_t non_minimal_hp = fastrange64(level_hash, _levels[level].hash_domain);
		return _levels[level].bitset.rank(non_minimal_hp);
	}

	[[nodiscard]] uint64_t nbKeys() const noexcept { return _nelem; }

	uint64_t totalBitSize()
	{
		uint64_t totalsizeBitset = 0;
		for (uint32_t ii = 0; ii < _nb_levels; ++ii)
		{
			totalsizeBitset += _levels[ii].bitset.bitSize();
		}

		const uint64_t totalsize = totalsizeBitset + _final_hash.size() * 42 * 8;

		std::cout << "Bitarray    " << totalsizeBitset << "  bits (" << std::fixed << std::setprecision(2)
		          << (100 * static_cast<float>(totalsizeBitset) / totalsize) << "% )   (array + ranks )\n";
		const uint64_t last_level_bits = static_cast<uint64_t>(_final_hash.size()) * 42ULL * 8ULL;
		std::cout << "Last level hash  " << last_level_bits << "  bits (" << std::fixed << std::setprecision(2)
		          << (100 * static_cast<float>(last_level_bits) / totalsize) << "% ) (nb in last level hash "
		          << _final_hash.size() << ")\n";
		return totalsize;
	}

	template <typename Iterator>
	void pthread_processLevel(std::vector<elem_t>& buffer, std::shared_ptr<Iterator> shared_it,
	                          std::shared_ptr<Iterator> until_p, int i)
	{
		uint64_t nb_done = 0;
		const uint32_t tid = _nb_living.fetch_add(1, std::memory_order_relaxed);
		auto until = *until_p;
		uint64_t inbuff = 0;

		uint64_t writebuff = 0;
		std::vector<elem_t>& myWriteBuff = bufferperThread[tid];

		for (bool isRunning = true; isRunning;)
		{
			// Safely copy items into buffer
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (; inbuff < NBBUFF && (*shared_it) != until; ++(*shared_it))
				{
					buffer[inbuff++] = *(*shared_it);
				}
				if ((*shared_it) == until)
				{
					isRunning = false;
				}
			}

			// Process buffered elements
			for (uint64_t ii = 0; ii < inbuff; ++ii)
			{
				const elem_t val = buffer[ii];

				hash_pair_t bbhash;
				int level;
				if (_writeEachLevel)
				{
					[[maybe_unused]] auto ignored = getLevel(bbhash, val, &level, i, i - 1);
				}
				else
				{
					[[maybe_unused]] auto ignored = getLevel(bbhash, val, &level, i);
				}

				if (level == i)
				{
					if (_fastmode && i == _fastModeLevel)
					{
						const uint64_t idxl2 = _idxLevelsetLevelFastmode.fetch_add(1, std::memory_order_relaxed);
						if (idxl2 >= static_cast<uint64_t>(setLevelFastmode.size()))
						{
							_fastmode = false;
						}
						else
						{
							setLevelFastmode[static_cast<size_t>(idxl2)] = val;
						}
					}

					// Insert into next level or final hash
					if (i == static_cast<int>(_nb_levels) - 1)
					{
						const uint64_t hashidx = _hashidx.fetch_add(1, std::memory_order_relaxed);
						std::lock_guard<std::mutex> lock(_mutex);
						_final_hash[val] = hashidx;
					}
					else
					{
						if (_writeEachLevel && i > 0 && i < static_cast<int>(_nb_levels) - 1)
						{
							if (writebuff >= NBBUFF)
							{
								write_with_file_lock(_currlevelFile, myWriteBuff, writebuff);
							}
						}

						// Compute next hash
						uint64_t level_hash;
						if (level == 0)
						{
							level_hash = _hasher.h0(bbhash, val);
						}
						else if (level == 1)
						{
							level_hash = _hasher.h1(bbhash, val);
						}
						else
						{
							level_hash = _hasher.next(bbhash);
						}
						insertIntoLevel(level_hash, i);
					}
				}

				++nb_done;
				if ((nb_done & 1023) == 0 && _withprogress)
				{
					_progressBar.inc(nb_done, tid);
					nb_done = 0;
				}
			}

			inbuff = 0;
		}

		if (_writeEachLevel && writebuff > 0)
		{
			write_with_file_lock(_currlevelFile, myWriteBuff, writebuff);
		}
	}

	void save(std::ostream& os) const
	{
		write_le(os, _gamma);
		write_le(os, _nb_levels);
		write_le(os, _lastbitsetrank);
		write_le(os, _nelem);

		for (uint32_t ii = 0; ii < _nb_levels; ++ii)
		{
			_levels[ii].bitset.save(os);
		}

		// Save final hash table
		const uint64_t final_hash_size = _final_hash.size();
		write_le(os, final_hash_size);

		for (const auto& kv_pair : _final_hash)
		{
			write_le(os, kv_pair.first);
			write_le(os, kv_pair.second);
		}
	}

	void load(std::istream& is)
	{
		read_le(is, _gamma);
		read_le(is, _nb_levels);
		read_le(is, _lastbitsetrank);
		read_le(is, _nelem);

		_levels.resize(_nb_levels);

		for (uint32_t ii = 0; ii < _nb_levels; ++ii)
		{
			_levels[ii].bitset.load(is);
		}

		// Recompute level parameters
		_proba_collision = 1.0 - std::pow(((_gamma * static_cast<double>(_nelem) - 1) /
		                                   (_gamma * static_cast<double>(_nelem))),
		                                  _nelem - 1);
		uint64_t previous_idx = 0;
		_hash_domain = static_cast<uint64_t>(std::ceil(static_cast<double>(_nelem) * _gamma));

		for (uint32_t ii = 0; ii < _nb_levels; ++ii)
		{
			_levels[ii].idx_begin = previous_idx;
			const double domain_d = static_cast<double>(_hash_domain) * std::pow(_proba_collision, ii);
			uint64_t domain = static_cast<uint64_t>(std::ceil(domain_d));
			_levels[ii].hash_domain = (domain + 63ULL) / 64ULL * 64ULL;
			if (_levels[ii].hash_domain == 0)
			{
				_levels[ii].hash_domain = 64ULL;
			}
			previous_idx += _levels[ii].hash_domain;
		}

		// Restore final hash table
		_final_hash.clear();
		uint64_t final_hash_size;
		read_le(is, final_hash_size);

		for (uint64_t ii = 0; ii < final_hash_size; ++ii)
		{
			elem_t key;
			uint64_t value;
			read_le(is, key);
			read_le(is, value);
			_final_hash[key] = value;
		}
		_built = true;
	}

private:
	void setup()
	{
		const uint64_t tid_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
		_pid = tid_hash;

		_cptTotalProcessed = 0;

		if (_fastmode)
		{
			setLevelFastmode.resize(static_cast<size_t>(_percent_elem_loaded_for_fastMode * static_cast<double>(_nelem)));
		}

		bufferperThread.resize(_num_thread);
		if (_writeEachLevel)
		{
			for (uint32_t ii = 0; ii < _num_thread; ++ii)
			{
				bufferperThread[ii].resize(NBBUFF);
			}
		}

		_proba_collision = 1.0 - std::pow(((_gamma * static_cast<double>(_nelem) - 1) /
		                                   (_gamma * static_cast<double>(_nelem))),
		                                  _nelem - 1);

		_nb_levels = 25;
		_levels.resize(_nb_levels);

		// Build level structures
		uint64_t previous_idx = 0;
		for (uint32_t ii = 0; ii < _nb_levels; ++ii)
		{
			_levels[ii].idx_begin = previous_idx;

			// Round size to nearest superior multiple of 64
			const double domain_d = static_cast<double>(_hash_domain) * std::pow(_proba_collision, ii);
			uint64_t domain = static_cast<uint64_t>(std::ceil(domain_d));
			_levels[ii].hash_domain = (domain + 63ULL) / 64ULL * 64ULL;
			if (_levels[ii].hash_domain == 0)
			{
				_levels[ii].hash_domain = 64ULL;
			}
			previous_idx += _levels[ii].hash_domain;
		}

		for (uint32_t ii = 0; ii < _nb_levels; ++ii)
		{
			if (std::pow(_proba_collision, ii) < _percent_elem_loaded_for_fastMode)
			{
				_fastModeLevel = ii;
				break;
			}
		}
	}

	/// Compute level for element and return hash of last level reached
	[[nodiscard]] uint64_t getLevel(hash_pair_t& bbhash, const elem_t& val, int* res_level,
	                                int maxlevel = 100, int minlevel = 0) const
	{
		int level = 0;
		uint64_t hash_raw = 0;

		for (uint32_t ii = 0; ii < (_nb_levels - 1) && ii < static_cast<uint32_t>(maxlevel); ++ii)
		{
			// Compute next hash
			if (ii == 0)
			{
				hash_raw = _hasher.h0(bbhash, val);
			}
			else if (ii == 1)
			{
				hash_raw = _hasher.h1(bbhash, val);
			}
			else
			{
				hash_raw = _hasher.next(bbhash);
			}
			if (ii >= static_cast<uint32_t>(minlevel) && _levels[ii].get(hash_raw))
			{
				break;
			}
			++level;
		}

		*res_level = level;
		return hash_raw;
	}

	/// Insert element into bit array level
	void insertIntoLevel(uint64_t level_hash, int i)
	{
		const uint64_t hashl = fastrange64(level_hash, _levels[i].hash_domain);
		if (_levels[i].bitset.atomic_test_and_set(hashl))
		{
			[[maybe_unused]] auto ignored = _tempBitset->atomic_test_and_set(hashl);
		}
	}

	/// Process elements at level i
	template <typename Range>
	void processLevel(const Range& input_range, int i)
	{
		_levels[i].bitset = bitVector(_levels[i].hash_domain);

		const std::string fname_old = "temp_p" + std::to_string(_pid) + "_level_" + std::to_string(i - 2) + ".tmp";
		const std::string fname_curr = "temp_p" + std::to_string(_pid) + "_level_" + std::to_string(i) + ".tmp";
		const std::string fname_prev = "temp_p" + std::to_string(_pid) + "_level_" + std::to_string(i - 1) + ".tmp";

		if (_writeEachLevel)
		{
			if (i > 2)
			{
				std::remove(fname_old.c_str());
			}

			if (i < static_cast<int>(_nb_levels) - 1 && i > 0)
			{
				_currlevelFile = std::fopen(fname_curr.c_str(), "w");
			}
		}

		_cptLevel = 0;
		_hashidx.store(0, std::memory_order_relaxed);
		_idxLevelsetLevelFastmode.store(0, std::memory_order_relaxed);
		_nb_living.store(0, std::memory_order_relaxed);

		// Create threads
		std::vector<std::thread> tab_threads;
		tab_threads.reserve(_num_thread);
		using it_type = decltype(input_range.begin());
		thread_args<Range, it_type> t_arg;
		t_arg.boophf = this;
		t_arg.range = &input_range;
		t_arg.it_p = std::static_pointer_cast<void>(std::make_shared<it_type>(input_range.begin()));
		t_arg.until_p = std::static_pointer_cast<void>(std::make_shared<it_type>(input_range.end()));
		t_arg.level = i;

		if (_writeEachLevel && (i > 1))
		{
			auto data_iterator_level = file_binary<elem_t>(fname_prev);
			using disklevel_it_type = decltype(data_iterator_level.begin());

			t_arg.it_p = std::static_pointer_cast<void>(std::make_shared<disklevel_it_type>(data_iterator_level.begin()));
			t_arg.until_p = std::static_pointer_cast<void>(std::make_shared<disklevel_it_type>(data_iterator_level.end()));

			for (uint32_t ii = 0; ii < _num_thread; ++ii)
			{
				// Create independent copy for each thread
				auto* my_arg = new thread_args<Range, it_type>(t_arg);
				tab_threads.emplace_back([my_arg]()
				                         {
                    thread_processLevel<elem_t, Hasher_t, Range, it_type>(my_arg);
                    delete my_arg; });
			}

			// Must join here before file_binary is destroyed
			for (auto& t : tab_threads)
			{
				t.join();
			}
		}
		else
		{
			if (_fastmode && i >= (_fastModeLevel + 1))
			{
				using fastmode_it_type = decltype(setLevelFastmode.begin());
				t_arg.it_p = std::static_pointer_cast<void>(std::make_shared<fastmode_it_type>(setLevelFastmode.begin()));
				t_arg.until_p = std::static_pointer_cast<void>(std::make_shared<fastmode_it_type>(setLevelFastmode.end()));

				for (uint32_t ii = 0; ii < _num_thread; ++ii)
				{
					auto* my_arg = new thread_args<Range, it_type>(t_arg);
					tab_threads.emplace_back([my_arg]()
					                         {
                        thread_processLevel<elem_t, Hasher_t, Range, it_type>(my_arg);
                        delete my_arg; });
				}
			}
			else
			{
				for (uint32_t ii = 0; ii < _num_thread; ++ii)
				{
					auto* my_arg = new thread_args<Range, it_type>(t_arg);
					tab_threads.emplace_back([my_arg]()
					                         {
                        thread_processLevel<elem_t, Hasher_t, Range, it_type>(my_arg);
                        delete my_arg; });
				}
			}

			for (auto& t : tab_threads)
			{
				t.join();
			}
		}

		if (_fastmode && i == _fastModeLevel)
		{
			setLevelFastmode.resize(_idxLevelsetLevelFastmode);
		}

		if (_writeEachLevel)
		{
			if (i < static_cast<int>(_nb_levels) - 1 && i > 0)
			{
				std::fflush(_currlevelFile);
				std::fclose(_currlevelFile);
			}

			if (i == static_cast<int>(_nb_levels) - 1)
			{
				std::remove(fname_prev.c_str());
			}
		}
	}

private:
	std::vector<level> _levels;
	uint32_t _nb_levels{0};
	MultiHasher_t _hasher;
	bitVector* _tempBitset{nullptr};

	double _gamma{2.0};
	uint64_t _hash_domain{0};
	uint64_t _nelem{0};
	std::unordered_map<elem_t, uint64_t, Hasher_t> _final_hash;
	Progress _progressBar;
	std::atomic<uint32_t> _nb_living{0};
	uint32_t _num_thread{1};
	std::atomic<uint64_t> _hashidx{0};
	double _proba_collision{0.0};
	uint64_t _lastbitsetrank{0};
	std::atomic<uint64_t> _idxLevelsetLevelFastmode{0};
	uint64_t _cptLevel{0};
	uint64_t _cptTotalProcessed{0};

	float _percent_elem_loaded_for_fastMode{0.03f};
	bool _fastmode{false};
	std::vector<elem_t> setLevelFastmode;

	std::vector<std::vector<elem_t>> bufferperThread;

	int _fastModeLevel{0};
	bool _withprogress{true};
	bool _built{false};
	bool _writeEachLevel{true};
	FILE* _currlevelFile{nullptr};
	uint64_t _pid{0};

public:
	std::mutex _mutex;
};

////////////////////////////////////////////////////////////////
// Threading
////////////////////////////////////////////////////////////////

template <typename elem_t, typename Hasher_t, typename Range, typename it_type>
void thread_processLevel(thread_args<Range, it_type>* targ)
{
	if (targ == nullptr)
	{
		return;
	}

	auto* obw = static_cast<mphf<elem_t, Hasher_t>*>(targ->boophf);
	const int level = targ->level;

	std::vector<elem_t> buffer(NBBUFF);

	std::shared_ptr<it_type> startit;
	std::shared_ptr<it_type> until_p;

	{
		std::lock_guard<std::mutex> lock(obw->_mutex);
		startit = std::static_pointer_cast<it_type>(targ->it_p);
		until_p = std::static_pointer_cast<it_type>(targ->until_p);
	}

	obw->pthread_processLevel(buffer, startit, until_p, level);
}

} // namespace boomphf
