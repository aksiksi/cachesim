#ifndef VICTIM_H
#define VICTIM_H

#include <deque>

#include "block.hpp"

class VictimCache {
public:
    VictimCache(u64 V);
    
    // Return index of block; -1 if not found
    int lookup(const u64 tag, const u64 index);

    bool replace(const int pos, const std::shared_ptr<Block> b2);
    bool push(const std::shared_ptr<Block>);
private:
    u64 V; // Number of blocks
    std::deque<std::shared_ptr<Block>> queue;
};

#endif
