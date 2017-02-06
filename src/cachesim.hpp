#ifndef CACHESIM_H
#define CACHESIM_H

typedef uint64_t u64;

void exit_on_error(std::string msg) {
    std::cout << "Error: " << msg << std::endl;
    exit(EXIT_FAILURE);
}

struct CacheSize { 
    u64 C, B, S, K, N;
};

/**
    Represents a single block in a cache.
*/
class Block {
public:
    // Valid and dirty bits for the block
    int valid = 1, dirty = 0;

    // Vector that stores subblocks
    // If value = 1, subblock is present
    std::vector<int> subblocks;
    int n; // Number of subblocks

    // Init subblocks to all 0s
    Block(u64 K) : subblocks(1 << K, 0), n(1 << K) {}

    // Read a single subblock
    bool read(int offset) {
        if (offset >= n)
            exit_on_error("Subblock index out of range.");

        if (subblocks[offset] == 1)
            return true;
        
        return false;
    }
    
    // Write a single sublock
    void write(int offset) {
        if (offset >= n)
            exit_on_error("Subblock index out of range.");

        subblocks[offset] = 1;
        dirty = 1;
    }

    // Write multiple subblocks (prefetch)
    void write_many(std::vector<int> &offsets) {
        for (auto idx: offsets)
            write(idx);
    }
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

    // LRU stack
    std::vector<Block*> LRU;

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

    bool read(u64 addr) {
        Block *b;
        bool hit = false;

        try {
            b = cache.at(get_tag(addr));
            LRU.push_back(b);
            hit = true;
        } catch (const std::out_of_range& e) {}

        return hit;
    }
    
    void write(u64 addr) {
        // TODO: incorporate replacement
        Block *b;
        auto search = cache.find(addr);
        
        if (search == cache.end()) {
            // Not found, create a new one
            b = new Block(size.K);
            cache.emplace(get_tag(addr), b);
        } else {
            b = search->second;

            int offset = get_offset(addr);
            b->write(offset);
        }
    }

private:
    inline u64 get_tag(u64 addr) {
        return addr & ~block_mask;
    }

    inline int get_offset(u64 addr) {
        return addr & block_mask;
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
