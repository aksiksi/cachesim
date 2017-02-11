// C++ includes
#include <iostream>

#include "cache.hpp"

Cache::Cache(CacheSize size, CacheType ct, cache_stats_t* cs) : 
            size(size), ct(ct), stats(cs) {
    u64 C = size.C, B = size.B, S = size.S, K = size.K;
    
    offset_mask = (1 << B) - 1;

    if (ct == DIRECT_MAPPED || ct == SET_ASSOC)
        index_mask = ((1 << (C-B-S)) - 1) << B;

    tag_mask = ~(index_mask | offset_mask);

    // Init the cache based on given parameters
    // First, figure out rows and cols for cache
    switch (ct) {
        case FULLY_ASSOC:
        case DIRECT_MAPPED:
            rows = (1 << (C-B));
            cols = 1;
            break;
        case SET_ASSOC:
            rows = (1 << (C-B-S));
            cols = (1 << S);
            break;
    }
    
    // Enable or disable subblocking
    bool sb = true;

    // Init cache
    // Set maximum size of vector for optimization
    cache.resize(rows, std::vector<std::shared_ptr<Block>>(cols));

    // Allocate a cache block to each vector position
    // Use shared_ptr for easy MM (RAII)
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            cache[i][j] = std::shared_ptr<Block>(new Block(B, K, sb));

    // Init lru
    lru.resize(rows, std::vector<u64>(cols, 0));

    #ifdef DEBUG
        std::cout << "Tag mask: " << std::hex << tag_mask << std::endl;
        std::cout << "Index mask: " << index_mask << std::endl;
        std::cout << "Offset mask: " << offset_mask << std::dec << std::endl;
        
        std::string t;

        if (ct == DIRECT_MAPPED)
            t = "Direct";
        else if (ct == FULLY_ASSOC)
            t = "Fully Associative";
        else
            t = "Set Associative";

        std::cout << "Cache type: " << t << std::endl;
    #endif

    stats->hit_time = 2 + 0.1 * (1 << S);
    stats->miss_penalty = 100;
}

CacheResult Cache::read(u64 addr) {
    u64 tag = get_tag(addr);
    u64 index = get_index(addr);
    u64 offset = get_offset(addr);

    // Add access to LRU
    lru_push(tag, index);

    stats->accesses++;
    stats->reads++;

    bool hit = false;
    std::shared_ptr<Block> block;

    if (ct == FULLY_ASSOC) {
        // FA cache -> search for tag directly
        for (auto each: cache) {
            auto b = each[0];

            // Check for tag
            if (b->tag == tag) {
                hit = true;
                block = b;
                break;
            }
        }
    } else if (ct == DIRECT_MAPPED) {
        // Retrieve the "only" possible block
        auto b = cache[index][0];

        if (b->tag == tag)
            hit = true;
    } else {
        // Set associative cache
        // Retrieve set based on index
        auto set = cache[index];

        for (auto b: set) {
            if (b->tag == tag) {
                hit = true;
                block = b;
                break;
            }
        }
    }

    if (hit) {
        // Subblock hit
        if (block->read(offset))
            return READ_HIT;
        // Subblock miss! -> Perform prefetch
        else {
            stats->subblock_misses++;

            // Only count invalid subblocks for prefetch
            int num_invalid = block->num_invalid(offset);
            stats->bytes_transferred += num_invalid;

            block->write_many(offset);
        
            return READ_SB_MISS;
        }
    } else {
        // Full read miss
        // Retrieve subblock and prefetch subsequent
        stats->read_misses++;
        
        // Find suitable victim to evict
        // Or return first empty block
        block = evict(tag, index);

        // Fetch required subblocks from memory
        int bytes = block->write_many(offset);
        stats->bytes_transferred += bytes;

        return READ_MISS;
    }
}

CacheResult Cache::write(u64 addr) {
    u64 tag = get_tag(addr);
    u64 index = get_index(addr);
    u64 offset = get_offset(addr);

    // lru_push(tag, index);

    stats->accesses++;
    stats->writes++;

    bool hit = false;
    std::shared_ptr<Block> block;

    if (ct == FULLY_ASSOC) {
        for (auto each: cache) {
            auto b = each[0];

            // Check for tag in cache
            if (b->tag == tag) {
                block = b;
                hit = true;
                break;
            }
        }
    } else if (ct == DIRECT_MAPPED) {
        auto b = cache[index][0];
        
        if (b->tag == tag)
            hit = true;
    } else {
        auto set = cache[index];

        for (auto b: set) {
            if (b->tag == tag) {
                hit = true;
                block = b;
                break;
            }
        }
    }

    if (hit) {
        block->write_many(offset);
        // block->replace(tag, true); // Full block replace
    } else {
        // Miss handled the same in all cases
        // Select a block, and evict it
        // If there is an empty spot, that is returned instead
        block = evict(tag, index);

        // Write data to it
        block->write_many(offset);

        stats->write_misses++;
    }

    block->dirty = true;

    if (hit)
        return WRITE_HIT;
    else
        return WRITE_MISS;
}

std::shared_ptr<Block>
Cache::find_victim(u64 tag, u64 index) {
    // Figure out candidate block for cache eviction
    // IF there are empty blocks, return first such one as a "victim"
    std::shared_ptr<Block> block;
    u64 victim_tag;
    
    if (ct == FULLY_ASSOC) {
        // Look for an empty block first 
        for (int i = 0; i < cache.size(); i++) {
            block = cache[i][0];
            if (block->tag == 0)
                return block;
        }

        // Now look for a victim
        victim_tag = lru_get(index);

        for (int i = 0; i < cache.size(); i++) {
            block = cache[i][0];
            if (block->tag == victim_tag)
                break;
        }
    } else if (ct == DIRECT_MAPPED) {
        block = cache[index][0];
    } else {
        auto set = cache[index];

        // Look for empty block first
        for (int i = 0; i < set.size(); i++) {
            block = set[i];
            if (block->tag == 0)
                return block;
        }

        // Time for a victim..
        victim_tag = lru_get(index);

        for (int i = 0; i < set.size(); i++) {
            block = set[i];
            if (block->tag == victim_tag)
                break;
        }
    }

    return block;
}

std::shared_ptr<Block>
Cache::evict(u64 tag, u64 index) {
    // Find block to use in cache
    // If block selected is dirty, write it to memory
    // Then replace it
    auto block = find_victim(tag, index);

    // Empty block => just ignore the eviction
    if (block->tag != 0) {
        // Replace the block in cache
        // Check if dirty first => writeback
        if (block->dirty) {
            // Write back to memory
            // int valid_bytes = block->num_valid();
            // stats->bytes_transferred += (1 << size.B);
            stats->bytes_transferred += (1 << size.B); // TRY
            stats->write_backs++;
        }

        // Read needed block from memory and write into cache
        // TODO: check if 2 * bytes required
        // stats->bytes_transferred += (1 << size.B);
    }

    // TODO: false vs true for replace all??
    block->replace(tag, false);
    
    return block;
}

void Cache::lru_push(u64 tag, u64 index) {
    if (ct == DIRECT_MAPPED)
        return;

    int idx = -1;
    int limit;
    int stack_size = 0;

    if (ct == SET_ASSOC) {
        // For SA, check set-local LRU
        auto stack = this->lru[index];
        stack_size = stack.size();

        for (int i = 0; i < stack_size; i++) {
            // Tag present in LRU
            if (stack[i] == tag) {
                idx = i;
                break;
            }
        }
    } else {
        // In case of FA cache
        // Need to adapt 2D vector to hold single lru

        // Any time a push happens, increment size of LRU
        lru_size = lru_size + 1;
        if (lru_size > lru.size())
            lru_size = lru.size();
        
        // First, find tag
        stack_size = this->lru_size;

        for (int i = 0; i < stack_size; i++) {
            if (lru[i][0] == tag) {
                idx = i;
                break;
            }
        }
    }

    // Tag already at top of stack
    if (idx == 0)
        return;

    if (idx != -1) {
        // Tag found => Repush
        // i.e., rotate until position of tag
        limit = idx;
    } else {
        // Simple push
        // Rotate right until the end, throw away last elem
        idx = 0;
        limit = stack_size-1;
    }

    u64 temp;

    if (ct == SET_ASSOC) {
        // Need reference to commit changes to original copy!
        auto& stack = lru[index];
        
        temp = stack[idx];

        for (int i = limit; i > 0; i--)
            stack[i+1] = stack[i];

        if (idx == 0)
            stack[0] = tag;
        else
            stack[0] = temp;
    } else {
        // Leverage 2nd dimension for FA cache
        temp = lru[idx][0];

        for (int i = limit; i > 0; i--)
            lru[i+1][0] = lru[i][0];

        if (idx == 0)
            lru[0][0] = tag;
        else
            lru[0][0] = temp;
    }
}

u64 Cache::lru_get(u64 index) {
    if (ct == DIRECT_MAPPED)
        return 0; // Error state
        
    // Get LRU tag (last element in stack)
    if (ct == SET_ASSOC) {
        auto stack = this->lru[index];
        return stack.back();
    } else {
        auto& last = this->lru[lru_size];
        return last[0];
    }
}
