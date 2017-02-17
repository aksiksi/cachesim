// C++ includes
#include <iostream>

#include "cache.hpp"

void LRU::push(u64 tag) {
    bool found = false;

    // Check if tag already in LRU
    for (u64 t: stack) {
        if (t == tag) {
            found = true;
            break;
        }
    }

    // Remove tag from stack
    if (found)
        stack.erase(std::remove(stack.begin(), stack.end(), tag), stack.end());

    // Do a push
    stack.push_front(tag);
}

u64 LRU::pop() {
    u64 tag = stack.back();
    stack.pop_back();
    return tag;
}

Cache::Cache(CacheSize size, CacheType ct, cache_stats_t* cs) : 
            size(size), ct(ct), stats(cs) {
    u64 C = size.C, B = size.B, S = size.S, K = size.K;
    
    offset_mask = static_cast<u64>(1 << B) - 1;
    index_mask = static_cast<u64>((1 << (C-B-S)) - 1) << B;
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
        default:
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
            cache[i][j] = std::make_shared<Block>(B, K, sb);

    // Init LRU
    for (int i = 0; i < rows; i++)
        lru.push_back(std::make_shared<LRU>());

    #if DEBUG
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
        for (auto& each: cache) {
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
        block = cache[index][0];

        if (block->tag == tag)
            hit = true;
    } else {
        // Set associative cache
        // Retrieve set based on index
        auto& set = cache[index];

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
            // Only count invalid subblocks for prefetch
            int num_invalid = block->num_invalid(offset);
            stats->bytes_transferred += num_invalid * (1 << size.K);

            block->write_many(offset);

            stats->subblock_misses++;
        
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

    lru_push(tag, index);

    stats->accesses++;
    stats->writes++;

    bool hit = false;
    std::shared_ptr<Block> block;

    if (ct == FULLY_ASSOC) {
        for (auto& each: cache) {
            auto b = each[0];

            // Check for tag in cache
            if (b->tag == tag) {
                block = b;
                hit = true;
                break;
            }
        }
    } else if (ct == DIRECT_MAPPED) {
        block = cache[index][0];
        
        if (block->tag == tag)
            hit = true;
    } else {
        auto& set = cache[index];

        for (auto& b: set) {
            if (b->tag == tag) {
                hit = true;
                block = b;
                break;
            }
        }
    }

    CacheResult cr;

    if (hit) {
        if (block->read(offset))
            cr = WRITE_HIT;
        else {
            // Subblock miss
            stats->subblock_misses++;

            int num_invalid = block->num_invalid(offset);
            stats->bytes_transferred += num_invalid * (1 << size.K);

            cr = WRITE_SB_MISS;
        }

        block->write_many(offset);
    } else {
        // Miss handled the same in all cases
        // Select a block, and evict it
        // If there is an empty spot, that is returned instead
        block = evict(tag, index);

        // Write data to it
        block->write_many(offset);

        // Retrieve block from memory
        stats->bytes_transferred += (1 << size.B); 
        stats->write_misses++;

        cr = WRITE_MISS;
    }

    block->dirty = true;

    return cr;
}

std::shared_ptr<Block>
Cache::evict(u64 tag, u64 index) {
    // Find block to use in cache
    // If block selected is dirty, write it to memory
    auto block = find_victim(index);

    // If empty block, just ignore the eviction
    if (block->tag != 0) {
        // Replace the block in cache
        // Check if dirty first => writeback
        if (block->dirty) {
            // Write back valid subblocks to memory
//            int valid = block->num_valid();
//            stats->bytes_transferred += valid * (1 << size.K);
            // TODO: check if correct!
            stats->bytes_transferred += (1 << size.B);
            stats->write_backs++;
        }
    }

    block->replace(tag, false);

    return block;
}

std::shared_ptr<Block>
Cache::find_victim(u64 index) {
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
        auto& set = cache[index];

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

void Cache::lru_push(u64 tag, u64 index) {
    if (ct == DIRECT_MAPPED)
        return;
    else if (ct == FULLY_ASSOC) {
        auto& l = lru[0];
        l->push(tag);
    } else {
        auto&l = lru[index];
        l->push(tag);
    }
}

u64 Cache::lru_get(u64 index) {
    if (ct == DIRECT_MAPPED)
        return 0; // Error state
        
    // Get LRU tag (last element in stack)
    if (ct == SET_ASSOC) {
        auto& l = lru[index];
        return l->pop();
    } else {
        auto& l = lru[0];
        return l->pop();
    }
}
