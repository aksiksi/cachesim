#ifndef CACHE_H
#define CACHE_H

#include <vector>
#include <forward_list>
#include <memory>

#include "block.hpp"
#include "cachesim.hpp"
#include "lru.hpp"
#include "victim.hpp"

#define DEBUG false

struct CacheSize {
    u64 C, B, S, K, V;
};

enum CacheType {
    DIRECT_MAPPED,
    FULLY_ASSOC,
    SET_ASSOC,
    VICTIM
};

CacheType find_cache_type(CacheSize size);

enum CacheResult {
    READ_HIT,
    READ_MISS,
    READ_SB_MISS,
    WRITE_MISS,
    WRITE_HIT,
    WRITE_SB_MISS
};

/*
    Maintains state for a single cache of any type.
*/
class Cache {
public:
    Cache(CacheSize size, CacheType ct, cache_stats_t* cs);
    ~Cache();

    CacheResult read(u64 addr);
    CacheResult write(u64 addr);

    void compute_stats();

private:
    u64 tag_mask = 0, index_mask = 0, offset_mask = 0;
    CacheSize size;
    CacheType ct;

    // Emulates cache: stores tag/idx -> block mapping
    std::vector<std::vector<Block>> cache;
    int rows, cols;

    // Check cache for specific block
    Block* find_block(const u64 tag, const u64 index);

    cache_stats_t* stats;

    Block* find_victim(u64 index);
    Block* evict(u64 tag, u64 index);

    // LRU stack
    std::vector<std::shared_ptr<LRU>> lru;
    void lru_push(u64 tag, u64 index);
    u64 lru_get(u64 index);

    // Victim cache
    bool vc = false;
    VictimCache* victim_cache;
    Block* check_vc(const u64 addr);

    // Cache index extraction
    inline u64 get_tag(u64 addr) {
        return addr & tag_mask;
    }

    inline u64 get_index(u64 addr) {
        return (addr & index_mask) >> size.B;
    }

    inline u64 get_offset(u64 addr) {
        return addr & offset_mask;
    }
};

#endif
