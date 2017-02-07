#ifndef CACHE_H
#define CACHE_H

#include <vector>

struct CacheSize {
    u64 C, B, S, K, N;
};

enum CacheType {
    DIRECT_MAPPED,
    FULLY_ASSOC,
    SET_ASSOC
};

/**
    Represents a single block in a cache.
*/
class Block {
public:
    // Vector that stores subblocks
    // If value = 1, subblock is present
    std::vector<int> subblocks;
    
    int n; // Number of subblocks
    u64 B; 

    u64 tag = 0;
    int valid = 0, dirty = 0;

    // Init subblocks
    Block(u64 B, u64 K);

    // Read a single subblock
    bool read(u64 offset);
    
    // Write a single sublock
    void write(u64 offset);

    // Write multiple subblocks (prefetch)
    int write_many(u64 offset);

    void replace(u64 tag);
};

/*
    Maintains state for a single cache of any type.
*/
class Cache {
public:
    CacheSize size;
    CacheType ct;
    u64 tag_mask = 0, index_mask = 0, block_mask = 0;

    // Emulates cache: stores tag/idx -> block mapping
    std::vector<std::vector<std::shared_ptr<Block>>> cache;
    int rows, cols;

    cache_stats_t* stats;

    // LRU stack
    // std::vector<u64> LRU;

    Cache(CacheSize size, cache_stats_t* cs);

    bool read(u64 addr);
    
    bool write(u64 addr);

private:
    inline u64 get_tag(u64 addr) {
        return addr & tag_mask;
    }

    inline u64 get_index(u64 addr) {
        return (addr & index_mask) >> size.B;
    }

    inline int get_offset(u64 addr) {
        return addr & block_mask;
    }

    inline u64 compute_key(u64 addr) {
        if (ct == CacheType::FULLY_ASSOC)
            return get_tag(addr);
        else
            return get_index(addr);
    }

    inline bool is_cache_full() {
        if (cache.size() == (1 << size.C))
            return true;
        
        return false;
    }
};

#endif