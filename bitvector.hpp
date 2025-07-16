#include <cstdlib>
#include <stdint.h>
#include <memory.h>

// #ifdef _WIN32
// #include <windows.h>
// #include <io.h>
// #endif

	
	
inline unsigned int popcount_32(unsigned int x)
{
    unsigned int m1 = 0x55555555;
    unsigned int m2 = 0x33333333;
    unsigned int m4 = 0x0f0f0f0f;
    unsigned int h01 = 0x01010101;
    x -= (x >> 1) & m1;               /* put count of each 2 bits into those 2 bits */
    x = (x & m2) + ((x >> 2) & m2);   /* put count of each 4 bits in */
    x = (x + (x >> 4)) & m4;          /* put count of each 8 bits in partie droite  4bit piece*/
    return (x * h01) >> 24;           /* returns left 8 bits of x + (x<<8) + ... */
}


inline unsigned int popcount_64(uint64_t x)
{
    unsigned int low = x & 0xffffffff ;
    unsigned int high = ( x >> 32LL) & 0xffffffff ;

    return (popcount_32(low) + popcount_32(high));
}


class bitVector {

public:

    bitVector() : _size(0)
    {
        _bitArray = nullptr;
    }

    bitVector(uint64_t n) : _size(n)
    {
        _nchar  = (1ULL+n/64ULL);
        _bitArray =  (uint64_t *) calloc (_nchar,sizeof(uint64_t));
    }

    ~bitVector()
    {
        if(_bitArray != nullptr)
            free(_bitArray);
    }

        //copy constructor
        bitVector(bitVector const &r)
        {
            _size =  r._size;
            _nchar = r._nchar;
            _ranks = r._ranks;
            _bitArray = (uint64_t *) calloc (_nchar,sizeof(uint64_t));
            memcpy(_bitArray, r._bitArray, _nchar*sizeof(uint64_t) );
        }
    
    // Copy assignment operator
    bitVector &operator=(bitVector const &r)
    {
        if (&r != this)
        {
            _size =  r._size;
            _nchar = r._nchar;
            _ranks = r._ranks;
            if(_bitArray != nullptr)
                free(_bitArray);
            _bitArray = (uint64_t *) calloc (_nchar,sizeof(uint64_t));
            memcpy(_bitArray, r._bitArray, _nchar*sizeof(uint64_t) );
        }
        return *this;
    }

    // Move assignment operator
    bitVector &operator=(bitVector &&r)
    {
        //printf("bitVector move assignment \n");
        if (&r != this)
        {
            if(_bitArray != nullptr)
                free(_bitArray);
            
            _size =  std::move (r._size);
            _nchar = std::move (r._nchar);
            _ranks = std::move (r._ranks);
            _bitArray = r._bitArray;
            r._bitArray = nullptr;
        }
        return *this;
    }
    // Move constructor
    bitVector(bitVector &&r) : _bitArray ( nullptr),_size(0)
    {
        *this = std::move(r);
    }
    
    
    void resize(uint64_t newsize)
    {
        //printf("bitvector resize from  %llu bits to %llu \n",_size,newsize);
        _nchar  = (1ULL+newsize/64ULL);
        _bitArray = (uint64_t *) realloc(_bitArray,_nchar*sizeof(uint64_t));
        _size = newsize;
    }

    size_t size() const
    {
        return _size;
    }

    uint64_t bitSize() const {return (_nchar*64ULL + _ranks.capacity()*64ULL );}

    //clear whole array
    void clear()
    {
        memset(_bitArray,0,_nchar*sizeof(uint64_t));
    }

    //clear collisions in interval, only works with start and size multiple of 64
    void clearCollisions(uint64_t start, size_t size, bitVector * cc)
    {
        assert( (start & 63) ==0);
        assert( (size & 63) ==0);
        uint64_t ids = (start/64ULL);
        for(uint64_t ii =0;  ii< (size/64ULL); ii++ )
        {
            _bitArray[ids+ii] =  _bitArray[ids+ii] & (~ (cc->get64(ii)) );
        }

        cc->clear();
    }


    //clear interval, only works with start and size multiple of 64
    void clear(uint64_t start, size_t size)
    {
        assert( (start & 63) ==0);
        assert( (size & 63) ==0);
        memset(_bitArray + (start/64ULL),0,(size/64ULL)*sizeof(uint64_t));
    }

    //for debug purposes
    void print() const
    {
        printf("bit array of size %lli: \n",(long long int)_size);
        for(uint64_t ii = 0; ii< _size; ii++)
        {
            if(ii%10==0)
                printf(" (%llu) ",(long long unsigned int)ii);
            int val = (_bitArray[ii >> 6] >> (ii & 63 ) ) & 1;
            printf("%i",val);
        }
        printf("\n");

        printf("rank array : size %zu \n",_ranks.size());
        for (uint64_t ii = 0; ii< _ranks.size(); ii++)
        {
            printf("%llu :  %lli,  ",(long long unsigned int)ii,(long long int)_ranks[ii]);
        }
        printf("\n");
    }

    //return value at pos
    uint64_t operator[](uint64_t pos) const
    {
        //unsigned char * _bitArray8 = (unsigned char *) _bitArray;
        //return (_bitArray8[pos >> 3ULL] >> (pos & 7 ) ) & 1;

        return (_bitArray[pos >> 6ULL] >> (pos & 63 ) ) & 1;

    }

    //atomically   return old val and set to 1
    // uint64_t atomic_test_and_set(uint64_t pos)
    // {
    // 	uint64_t oldval = 	__sync_fetch_and_or (_bitArray + (pos >> 6), (uint64_t) (1ULL << (pos & 63)) );

    // 	return  ( oldval >> (pos & 63 ) ) & 1;
    // }
    inline uint64_t atomic_test_and_set(uint64_t pos)
    {
        uint64_t mask = 1ULL << (pos & 63);
        std::atomic<uint64_t>* word = reinterpret_cast<std::atomic<uint64_t>*>(_bitArray + (pos >> 6));
        uint64_t oldval = word->load(std::memory_order_seq_cst);
        while (!word->compare_exchange_weak(oldval, oldval | mask, std::memory_order_seq_cst)) {}
        return (oldval >> (pos & 63)) & 1;
    }



    uint64_t get(uint64_t pos) const
    {
        return (*this)[pos];
    }

    uint64_t get64(uint64_t cell64) const
    {
        return _bitArray[cell64];
    }

    //set bit pos to 1
    void set(uint64_t pos)
    {
        assert(pos<_size);
        //_bitArray [pos >> 6] |=   (1ULL << (pos & 63) ) ;
        #ifdef _WIN32
        InterlockedOr64((volatile LONG64*)(_bitArray + (pos >> 6ULL)), (1ULL << (pos & 63)));
        #else
        __sync_fetch_and_or(_bitArray + (pos >> 6ULL), (1ULL << (pos & 63)));
        #endif
    }

    //set bit pos to 0
    void reset(uint64_t pos)
    {
        //_bitArray [pos >> 6] &=   ~(1ULL << (pos & 63) ) ;
        #ifdef _WIN32
        InterlockedAnd64((volatile LONG64*)(_bitArray + (pos >> 6ULL)), ~(1ULL << (pos & 63)));
        #else
        __sync_fetch_and_and(_bitArray + (pos >> 6ULL), ~(1ULL << (pos & 63)));
        #endif
    }

    //return value of  last rank
    // add offset to  all ranks  computed
    uint64_t build_ranks(uint64_t offset =0)
    {
        _ranks.reserve(2+ _size/_nb_bits_per_rank_sample);

        uint64_t curent_rank = offset;
        for (size_t ii = 0; ii < _nchar; ii++) {
            if (((ii*64)  % _nb_bits_per_rank_sample) == 0) {
                _ranks.push_back(curent_rank);
            }
            curent_rank +=  popcount_64(_bitArray[ii]);
        }

        return curent_rank;
    }

    uint64_t rank(uint64_t pos) const
    {
        uint64_t word_idx = pos / 64ULL;
        uint64_t word_offset = pos % 64;
        uint64_t block = pos / _nb_bits_per_rank_sample;
        uint64_t r = _ranks[block];
        for (uint64_t w = block * _nb_bits_per_rank_sample / 64; w < word_idx; ++w) {
            r += popcount_64( _bitArray[w] );
        }
        uint64_t mask = (uint64_t(1) << word_offset ) - 1;
        r += popcount_64( _bitArray[word_idx] & mask);

        return r;
    }



    void save(std::ostream& os) const
    {
        os.write(reinterpret_cast<char const*>(&_size), sizeof(_size));
        os.write(reinterpret_cast<char const*>(&_nchar), sizeof(_nchar));
        os.write(reinterpret_cast<char const*>(_bitArray), (std::streamsize)(sizeof(uint64_t) * _nchar));
        uint64_t sizer = _ranks.size();
        os.write(reinterpret_cast<char const*>(&sizer),  sizeof(uint64_t));
        os.write(reinterpret_cast<char const*>(_ranks.data()), (std::streamsize)(sizeof(_ranks[0]) * _ranks.size()));
    }

    void load(std::istream& is)
    {
        is.read(reinterpret_cast<char*>(&_size), sizeof(_size));
        is.read(reinterpret_cast<char*>(&_nchar), sizeof(_nchar));
        this->resize(_size);
        is.read(reinterpret_cast<char *>(_bitArray), (std::streamsize)(sizeof(uint64_t) * _nchar));

        uint64_t sizer;
        is.read(reinterpret_cast<char *>(&sizer),  sizeof(uint64_t));
        _ranks.resize(sizer);
        is.read(reinterpret_cast<char*>(_ranks.data()), (std::streamsize)(sizeof(_ranks[0]) * _ranks.size()));
    }


protected:
    uint64_t*  _bitArray;
    uint64_t _size;
    uint64_t _nchar;

        // epsilon =  64 / _nb_bits_per_rank_sample   bits
    // additional size for rank is epsilon * _size
    static const uint64_t _nb_bits_per_rank_sample = 512; //512 seems ok
    std::vector<uint64_t> _ranks;
};