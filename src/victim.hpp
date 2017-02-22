#ifndef VICTIM_H
#define VICTIM_H

#include <deque>

#include "block.hpp"

class VictimCache {
public:
    VictimCache(u64 V) : V(V) {}
    
    // Return index of block; -1 if not found
    int lookup(const u64 tag, const u64 index);

    // Remove block from VC at position `pos`
    // temp is used to pass block data back to caller
    bool remove(const int pos, Block* temp);

    // Push a block onto VC
    // Remove last if size > V
    // stats* required to update write_backs
    bool push(const std::shared_ptr<Block>, cache_stats_t* stats);
private:
    u64 V; // Number of blocks
    std::deque<Block> queue;
};

#endif
