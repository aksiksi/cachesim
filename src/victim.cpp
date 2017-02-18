#include "cachesim.hpp"
#include "victim.hpp"

 VictimCache::VictimCache(u64 V) : V(V) {
    //  for (int i = 0; i < )
 }

int VictimCache::lookup(const u64 tag, const u64 index) {
    // Check if block currently in VC
    for (int i = 0; i < queue.size(); i++) {
        auto block = queue[i];
     
        if (block->tag == tag && block->index == index)
            return i;
    }
    
    return -1;
}

bool VictimCache::replace(const int pos, const std::shared_ptr<Block> b2) {
    // TODO: need to create copy of the block to replace etc.

    // Replace b1 -- which is in the VC! -- with b2
    if (pos == -1)
        exit_on_error("VC block replacement failed!");
    
    // Remove old block and place new one
    queue.erase(queue.begin() + pos);

    // "Requeue" new block in FIFO (if non-empty)
    if (b2->tag != 0)
        this->push(b2);

    return true;
}

bool VictimCache::push(const std::shared_ptr<Block> block) {
    // TODO: need to create copy of the block to push onto VC
    // Push block to front of queue
    queue.push_front(block);

    // Delete last element, if required
    if (queue.size() > V)
        queue.pop_back();

    return false;
}
