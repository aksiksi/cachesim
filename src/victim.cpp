#include "cachesim.hpp"
#include "victim.hpp"

#include <iostream>

int VictimCache::lookup(const u64 tag, const u64 index) {
    int i;

    for (i = 0; i < queue.size(); i++) {
        Block& block = queue[i];
        if (block.tag == tag && block.index == index) {
            return i;
        }
    }

    return -1;
}

Block* VictimCache::remove(const int pos) {
    // pos: result of lookup for some block
    if (pos == -1)
        exit_on_error("VC block replacement failed!");
    
    // Save reference to block
    Block* temp = new Block(0, 0, 0);
    *temp = queue[pos];

    // Remove block from VC
    queue.erase(queue.begin() + pos);

    // "Requeue" new block in FIFO (if non-empty)
    // if (block->tag != 0)
    //     this->push(block);

    return temp;
}

void VictimCache::push(const std::shared_ptr<Block> block, cache_stats_t* stats, u64 K) {
    // Push block to front of queue
    // Vector will make a copy of the block passed in
    queue.push_front(*block);

    // Perform eviction, if required
    if (queue.size() > V) {
        Block &out_block = queue[V];

        // Writeback if target is dirty
        if (out_block.dirty) {
            // Write back valid subblocks to memory
            int valid = block->num_valid();
            stats->bytes_transferred += valid * (1 << K);
            stats->write_backs++;
        }

        queue.pop_back();
    }
}
