#include "cachesim.hpp"
#include "victim.hpp"
#include "block.hpp"

#include <iostream>

int VictimCache::lookup(const u64 tag, const u64 index) {
    // Check if block currently in VC
    for (int i = 0; i < queue.size(); i++) {
        Block& block = queue[i];
     
        if (block.tag == tag && block.index == index)
            return i;
    }
    
    return -1;
}

bool VictimCache::remove(const int pos, Block* temp) {
    // pos: result of lookup for some block
    // block: ptr to block to insert into VC

    // Replace b1 -- which is in the VC! -- with b2
    if (pos == -1)
        exit_on_error("VC block replacement failed!");
    
    // Remove old block and save reference
    *temp = queue[pos];
    queue.erase(queue.begin() + pos);

    // "Requeue" new block in FIFO (if non-empty)
    // if (block->tag != 0)
    //     this->push(block);

    return true;
}

bool VictimCache::push(const std::shared_ptr<Block> block, cache_stats_t* stats) {
    // Push block to front of queue
    // Vector will make a copy of the block passed in
    queue.push_front(*block);

    // Delete last element, if required
    // Also, writeback if target is dirty
    if (queue.size() > V) {
        auto& out_block = queue[V];
        
        if (out_block.dirty) {
            std::cout << "HERE";
            // TODO: check if correct!
            stats->bytes_transferred += (1 << out_block.B);
            stats->write_backs++;
        }

        queue.pop_back();
    }

    return false;
}
