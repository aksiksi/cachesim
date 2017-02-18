#ifndef CACHE_H
#define CACHE_H

#include <vector>
#include <deque>

#include "cachesim.hpp"
#include "block.hpp"
#include "victim.hpp"

#define DEBUG false

struct CacheSize {
    u64 C, B, S, K, N, V;
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

class LRU {
public:
    LRU() {}
    void push(u64 tag);
    u64 pop();
private:
    std::deque<u64> stack;
};

/*
    Maintains state for a single cache of any type.
*/
class Cache {
public:
    Cache(CacheSize size, CacheType ct, cache_stats_t* cs, bool vc);
    ~Cache();

    CacheResult read(u64 addr);
    CacheResult write(u64 addr);

private:
    u64 tag_mask = 0, index_mask = 0, offset_mask = 0;
    CacheSize size;
    CacheType ct;

    // Emulates cache: stores tag/idx -> block mapping
    std::vector<std::vector<std::shared_ptr<Block>>> cache;
    int rows, cols;

    cache_stats_t* stats;

    std::shared_ptr<Block> find_victim(u64 index);
    std::shared_ptr<Block> evict(u64 tag, u64 index);

    // LRU stack
    std::vector<std::shared_ptr<LRU>> lru;
    void lru_push(u64 tag, u64 index);
    u64 lru_get(u64 index);

    // Victim cache
    bool vc = false;
    VictimCache *victim_cache;

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
