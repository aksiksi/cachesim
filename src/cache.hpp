#ifndef CACHE_H
#define CACHE_H

#include <vector>

#include "cachesim.hpp"
#include "block.hpp"

#define DEBUG true

struct CacheSize {
    u64 C, B, S, K, N;
};

enum CacheType {
    DIRECT_MAPPED,
    FULLY_ASSOC,
    SET_ASSOC,
    VICTIM
};

enum CacheResult {
    READ_HIT,
    READ_MISS,
    READ_SB_MISS,
    WRITE_MISS,
    WRITE_HIT,
    WRITE_SB_MISS
};

class VictimCache {
public:
    u64 B, V;
    u64 tag_mask = 0, offset_mask = 0;
    
    std::vector<std::vector<std::shared_ptr<Block>>> cache;
    int rows, cols;

    cache_stats_t* stats;

    VictimCache(u64 B, u64 V);

    bool read(u64 addr);

    bool write(u64 addr);

private:
    inline int get_offset(u64 addr) {
        return addr & offset_mask;
    }

    inline u64 get_tag(u64 addr) {
        return addr & tag_mask;
    }
};

/*
    Maintains state for a single cache of any type.
*/
class Cache {
public:
    CacheSize size;
    CacheType ct;
    u64 tag_mask = 0, index_mask = 0, offset_mask = 0;

    // Emulates cache: stores tag/idx -> block mapping
    std::vector<std::vector<std::shared_ptr<Block>>> cache;
    int rows, cols;

    cache_stats_t* stats;

    // LRU stack
    // 2D vector of tags
    std::vector<std::vector<u64>> lru;
    std::size_t lru_size = 0; // For FA cache ONLY

    Cache(CacheSize size, CacheType ct, cache_stats_t* cs);

    CacheResult read(u64 addr);
    CacheResult write(u64 addr);

private:
    std::shared_ptr<Block> find_victim(u64 tag, u64 index);
    std::shared_ptr<Block> evict(u64 tag, u64 index);

    // LRU methods
    void lru_push(u64 tag, u64 index);
    u64 lru_get(u64 index);
    
    inline u64 get_tag(u64 addr) {
        return addr & tag_mask;
    }

    inline u64 get_index(u64 addr) {
        return (addr & index_mask) >> size.B;
    }

    inline int get_offset(u64 addr) {
        return addr & offset_mask;
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