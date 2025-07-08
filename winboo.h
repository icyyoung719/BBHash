#pragma once
#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstdint>
#include <istream>

template<typename elem_t>
class bitVector {
public:
    uint64_t _size = 0;
    uint64_t _nchar = 0;
    uint64_t* _bitArray = nullptr;
    std::vector<uint64_t> _ranks;

    ~bitVector() {
        delete[] _bitArray;
    }

    void resize(uint64_t size) {
        _size = size;
        _nchar = (size + 63) / 64;
        delete[] _bitArray;
        _bitArray = new uint64_t[_nchar]();
    }

    void load(std::istream& is) {
        is.read(reinterpret_cast<char*>(&_size), sizeof(_size));
        is.read(reinterpret_cast<char*>(&_nchar), sizeof(_nchar));
        resize(_size);
        is.read(reinterpret_cast<char*>(_bitArray), sizeof(uint64_t) * _nchar);
        size_t rank_size;
        is.read(reinterpret_cast<char*>(&rank_size), sizeof(rank_size));
        _ranks.resize(rank_size);
        is.read(reinterpret_cast<char*>(_ranks.data()), sizeof(uint64_t) * rank_size);
    }
};

template<typename elem_t>
struct level {
    bitVector<elem_t> bitset;
    uint64_t idx_begin;
    uint64_t hash_domain;
};

template<typename elem_t>
class mphf {
public:
    mphf() : _built(false)
    {}

    uint64_t lookup(const elem_t &elem)
    {
        if(! _built) return ULLONG_MAX;
        
        //auto hashes = _hasher(elem);
        uint64_t non_minimal_hp,minimal_hp;


        hash_pair_t bbhash;  int level;
        uint64_t level_hash = getLevel(bbhash,elem,&level);

        if( level == (_nb_levels-1))
        {
            auto in_final_map  = _final_hash.find (elem);
            if ( in_final_map == _final_hash.end() )
            {
                //elem was not in orignal set of keys
                return ULLONG_MAX; //  means elem not in set
            }
            else
            {
                minimal_hp =  in_final_map->second + _lastbitsetrank;
                //printf("lookup %llu  level %i   --> %llu \n",elem,level,minimal_hp);

                return minimal_hp;
            }
//				minimal_hp = _final_hash[elem] + _lastbitsetrank;
//				return minimal_hp;
        }
        else
        {
            //non_minimal_hp =  level_hash %  _levels[level].hash_domain; // in fact non minimal hp would be  + _levels[level]->idx_begin
            non_minimal_hp = fastrange64(level_hash,_levels[level].hash_domain);
        }
        minimal_hp = _levels[level].bitset.rank(non_minimal_hp );
    //	printf("lookup %llu  level %i   --> %llu \n",elem,level,minimal_hp);

        return minimal_hp;
		}


    uint64_t totalBitSize()
    {

        uint64_t totalsizeBitset = 0;
        for(int ii=0; ii<_nb_levels; ii++)
        {
            totalsizeBitset += _levels[ii].bitset.bitSize();
        }

        uint64_t totalsize =  totalsizeBitset +  _final_hash.size()*42*8 ;  // unordered map takes approx 42B per elem [personal test] (42B with uint64_t key, would be larger for other type of elem)

        printf("Bitarray    %" PRIu64 "  bits (%.2f %%)   (array + ranks )\n",
                totalsizeBitset, 100*(float)totalsizeBitset/totalsize);
        printf("Last level hash  %12lu  bits (%.2f %%) (nb in last level hash %lu)\n",
                _final_hash.size()*42*8, 100*(float)(_final_hash.size()*42*8)/totalsize,
                _final_hash.size() );
        return totalsize;
    }

    bool load(std::istream& is)
    {
        is.read(reinterpret_cast<char*>(&_gamma), sizeof(_gamma));
        is.read(reinterpret_cast<char*>(&_nb_levels), sizeof(_nb_levels));
        is.read(reinterpret_cast<char*>(&_lastbitsetrank), sizeof(_lastbitsetrank));
        is.read(reinterpret_cast<char*>(&_nelem), sizeof(_nelem));

        _levels.resize(_nb_levels);
        for (int ii = 0; ii < _nb_levels; ++ii) {
            _levels[ii].bitset.load(is);
        }

        _proba_collision = 1.0 - std::pow(((_gamma * (double)_nelem - 1) / (_gamma * (double)_nelem)), _nelem - 1);
        _hash_domain = static_cast<uint64_t>(std::ceil(_gamma * _nelem));
        uint64_t previous_idx = 0;
        for (int ii = 0; ii < _nb_levels; ++ii) {
            _levels[ii].idx_begin = previous_idx;
            _levels[ii].hash_domain = ((uint64_t)(_hash_domain * std::pow(_proba_collision, ii)) + 63) / 64 * 64;
            if (_levels[ii].hash_domain == 0)
                _levels[ii].hash_domain = 64;
            previous_idx += _levels[ii].hash_domain;
        }

        size_t final_hash_size = 0;
        is.read(reinterpret_cast<char*>(&final_hash_size), sizeof(final_hash_size));
        for (size_t i = 0; i < final_hash_size; ++i) {
            elem_t key;
            uint64_t value;
            is.read(reinterpret_cast<char*>(&key), sizeof(key));
            is.read(reinterpret_cast<char*>(&value), sizeof(value));
            _final_hash[key] = value;
        }

        _built = true;
        return true;
    }


    // 添加查询函数等 ...
    uint64_t size() const { return _nelem; }

private:
    std::vector<level<elem_t>> _levels;
    int _nb_levels = 0;
    double _gamma = 0.0;
    uint64_t _hash_domain = 0;
    uint64_t _nelem = 0;
    std::unordered_map<elem_t, uint64_t> _final_hash;
    double _proba_collision = 0.0;
    uint64_t _lastbitsetrank = 0;
    bool _built = false;
};
