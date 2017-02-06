#ifndef CACHESIM_H
#define CACHESIM_H

typedef uint64_t u64;

struct CacheSize { 
    u64 C, B, S, K, N;
};

/**
    Represents a single block in a cache.
*/
class Block {
public:
    // Valid and dirty bits for the block
    int valid = 0, dirty = 0;

    // Vector that stores subblock indices
    std::vector<int> subblocks;
    
    Block() {}
    Block(u64 K) : subblocks(1 << K) {}
};

/*
    Maintains state for a single cache of any type.
*/
class Cache {
public:
    CacheSize size;
    u64 block_mask;

    // Emulates cache: stores tag -> block mapping
    std::unordered_map<u64, Block*> cache;

    Cache(CacheSize size): size(size) {
        block_mask = 0;
        for (int i = 0; i < size.B; i++)
            block_mask |= (1 << i);
    }

    ~Cache() {
        // Free allocated blocks
        for (auto b: cache)
            delete b.second;
    }

    void read(u64 addr) {
        Block *b;

        try {
            b = cache.at(get_tag(addr));
            std::cout << "HIT: " << addr << std::endl;
        } catch (const std::out_of_range& e) {
            std::cout << "MISS: " << addr << std::endl;
        }
    }
    
    void write(u64 addr) {
        Block *b = new Block(size.K);
        b->valid = 1;
        cache.emplace(get_tag(addr), b);
    }

private:
    inline u64 get_tag(u64 addr) {
        return addr & ~block_mask;
    }
};

struct cache_stats_t {
    uint64_t accesses;

    uint64_t reads;
    uint64_t read_misses;
    uint64_t read_misses_combined;

    uint64_t writes;
    uint64_t write_misses;
    uint64_t write_misses_combined;

    uint64_t misses;
	uint64_t write_backs;
	uint64_t vc_misses;

    uint64_t subblock_misses;

	uint64_t bytes_transferred; 
   
	uint64_t hit_time;
    uint64_t miss_penalty;
    double   miss_rate;
    double   avg_access_time;
};

static const uint64_t DEFAULT_C = 15;   /* 32KB Cache */
static const uint64_t DEFAULT_B = 6;    /* 64-byte blocks */
static const uint64_t DEFAULT_S = 3;    /* 8 blocks per set */
static const uint64_t DEFAULT_V = 4;    /* 4 victim blocks */
static const uint64_t DEFAULT_K = 3;	/* 8 byte sub-blocks */

/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_access rw. Indicates a store */
static const char     WRITE = 'w';

#endif /* CACHESIM_H */
