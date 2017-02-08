// C++ includes
#include <iostream>

#include "cachesim.hpp"
#include "cache.hpp"

Block::Block(u64 B, u64 K, bool sb) : B(B), sb(sb) {
    // Skip init if no subblocking
    if (!sb) return;
    
    // Number of subblocks = 2^B / 2^K
    n = (1 << (B-K));
    valid.reserve(n);
    std::fill(valid.begin(), valid.end(), 0);
}

// Read a single subblock
bool Block::read(u64 offset) {
    // No subblocking = always hit
    if (!sb) return true;
    
    // Figure out which subblock is being requested
    int idx = offset % n;
    if (idx > n)
        exit_on_error("Subblock index out of range.");

    if (valid[idx] == 1)
        return true;
    
    return false;
}

void Block::write(u64 offset) {
    if (!sb) return;
    
    int idx = offset % n;
    if (idx > n)
        exit_on_error("Subblock index out of range.");

    valid[idx] = 1;
}

// Write multiple subblocks (prefetch)
// Returns number of bytes written
int Block::write_many(u64 offset) {
    // If no subblocking, always write entire block
    if (!sb) return (1 << B);
    
    int c = 0;
    
    // Iterate through all offsets
    for (; offset < (1 << B); offset++, c++)
        write(offset);

    return c;
}

void Block::replace(u64 tag) {
    this->tag = tag;
    this->dirty = false;

    // Full block replace => all valid
    std::fill(valid.begin(), valid.end(), 1);
}

void Block::empty() {
    // TODO
    tag = 0;
    dirty = false;
    std::fill(valid.begin(), valid.end(), 0);
}

Cache::Cache(CacheSize size, CacheType ct, cache_stats_t* cs) : 
            size(size), ct(ct), stats(cs) {
    u64 C = size.C, B = size.B, S = size.S, K = size.K;
    
    offset_mask = (1 << B) - 1;

    if (ct == CacheType::DIRECT_MAPPED || ct == CacheType::SET_ASSOC)
        index_mask = ((1 << (C-B-S)) - 1) << B;

    tag_mask = ~(index_mask | offset_mask);

    // Init the cache based on given parameters
    // First, figure out rows and cols for cache
    switch (ct) {
        case CacheType::FULLY_ASSOC:
        case CacheType::DIRECT_MAPPED:
            rows = (1 << (C-B));
            cols = 1;
            break;
        case CacheType::SET_ASSOC:
            rows = (1 << (C-B-S));
            cols = (1 << S);
            break;
        // case CacheType::VICTIM:
        //     rows = (1 << (V-B));
        //     cols = 1;
        //     break;
    }
    
    // Enable or disable subblocking
    bool sb = true;

    // Set maximum size of vector for optimization
    cache.resize(rows, std::vector<std::shared_ptr<Block>>(cols));

    // Allocate a cache block to each vector position
    // Use shared_ptr for easy MM (RAII)
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            cache[i][j] = std::shared_ptr<Block>(new Block(B, K, sb));

    #ifdef DEBUG
        std::cout << "Tag mask: " << std::hex << tag_mask << std::endl;
        std::cout << "Index mask: " << index_mask << std::endl;
        std::cout << "Offset mask: " << offset_mask << std::dec << std::endl;
        
        std::string t;

        if (ct == CacheType::DIRECT_MAPPED)
            t = "Direct";
        else if (ct == CacheType::FULLY_ASSOC)
            t = "Fully Associative";
        else
            t = "Set Associative";

        std::cout << "Cache type: " << t << std::endl;
    #endif
}

int Cache::read(u64 addr) {
    // TODO
    // 1. Add sublock miss support
    u64 tag = get_tag(addr);
    u64 index = get_index(addr);
    u64 offset = get_offset(addr);

    stats->accesses++;
    stats->reads++;

    bool hit = false;
    std::shared_ptr<Block> block;

    if (ct == CacheType::FULLY_ASSOC) {
        // FA cache -> search for tag directly
        for (auto each: cache) {
            auto b = each[0];

            // Check for tag in cache
            if (b->tag == tag) {
                hit = true;
                block = b;

                // // Check for subblock
                // if (block->read(offset))
                //     return 0; // Hit!
                // // Subblock miss! -> Perform prefetch
                // else {
                //     // Retrieve current and all following from memory
                //     stats->subblock_misses++;
                //     int bytes = block->write_many(offset);
                //     stats->bytes_transferred += bytes;
                //     return bytes; // Number of bytes fetched
                // }
            }
        }

        // Block not found, retrieve from memory and store in cache
        stats->read_misses++;

        // Retrieve block and write to cache
        // TODO: Replace with write()
        auto block = cache[0][0]; // TODO: better replace (global LRU)
        block->replace(tag);
        int bytes = (1 << size.B);
        stats->bytes_transferred += bytes;
        stats->accesses++;
        return bytes;
    } else if (ct == CacheType::DIRECT_MAPPED) {
        // Retrieve the "only" possible block
        auto b = cache[index][0];

        if (b->tag == tag) {
            hit = true;
            block = b;
        }
        else {
            // Block miss!
            stats->read_misses++;
            block->replace(tag);
            stats->bytes_transferred += (1 << size.B);
            stats->accesses++;
            return (1 << size.B);
        }
    } else {
        // Set associative cache
        // Retrieve set based on index
        auto set = cache[index];

        for (auto b: set) {
            if (b->tag == tag) {
                hit = true;
                block = b;
            }
        }

        // Block not found, retrieve from memory and store in cache
        stats->read_misses++;

        // Evict first block in set
        // TODO: Use write()
        auto block = set[0]; // TODO: better replace (based on LRU)
        block->replace(tag);
        int bytes = (1 << size.B);
        stats->bytes_transferred += bytes;
        stats->accesses++;
        return bytes;
    }

    if (hit) {
        // Check for subblock
        if (block->read(offset))
            return 0; // Hit!
        // Subblock miss! -> Perform prefetch
        else {
            // Retrieve current and all following from memory
            stats->subblock_misses++;
            int bytes = block->write_many(offset);
            stats->bytes_transferred += bytes;
            return bytes; // Number of bytes fetched
        }
    } else {
        // Full miss
        // Retrieve subblock and prefetch subsequent
        stats->read_misses++;
        
        block = find_victim(tag, index);
        
        // Write back if dirty
        if (block->dirty) {
            stats->bytes_transferred += (1 << size.B);
            stats->write_backs++;
        }

        // Empty block and fetch required subblocks
        block->empty();
        block->tag = tag;

        int bytes = block->write_many(offset);
        stats->bytes_transferred += bytes;

        return bytes;
    }
}

bool Cache::write(u64 addr) {
    // TODO: incorporate replacement
    // Replacement differs:
    // DM: MUST place at tag position
    // FA: can place anywhere (use LRU stack)
    // SA: MUST place in correct set (use index mask)
    u64 tag = get_tag(addr);
    u64 index = get_index(addr);

    stats->accesses++;
    stats->writes++;

    bool hit = false;

    std::shared_ptr<Block> block;

    if (ct == CacheType::FULLY_ASSOC) {
        for (auto each: cache) {
            auto b = each[0];

            // Check for tag in cache
            if (b->tag == tag) {
                hit = true;
                block = b;
            }
        }
    } else if (ct == CacheType::DIRECT_MAPPED) {
        auto b = cache[index][0];
        
        if (b->tag == tag) {
            hit = true;
            block = b;
        }
    } else {
        auto set = cache[index];

        for (auto b: set) {
            if (b->tag == tag) {
                hit = true;
                block = b;
            }
        }
    }

    if (hit) {
        block->replace(tag);
        stats->bytes_transferred += (1 << size.B); // TODO: check this value
    } else {
        // Miss handled the same in all cases
        // Select a block, and evict it
        evict(tag, index);
        stats->write_misses++;
    }

    block->dirty = true;

    return hit;
}

std::shared_ptr<Block>
Cache::find_victim(u64 tag, u64 index) {
    std::shared_ptr<Block> block;

    // Figure out candidate block for eviction
    if (ct == CacheType::FULLY_ASSOC) {
        block = cache[0][0]; // TODO: FIX
    } else if (ct == CacheType::DIRECT_MAPPED) {
        block = cache[index][0];
    } else {
        block = cache[index][0]; // TODO: FIX
    }

    return block;
}

void Cache::evict(u64 tag, u64 index) {
    // Find block to use in cache
    // If block selected is dirty, write it to memory
    // Then replace it
    auto block = find_victim(tag, index);
    
    // Replace the block in cache
    // Check if dirty first => writeback
    if (block->dirty) {
        // Write back to memory
        stats->bytes_transferred += (1 << size.B);
        stats->write_backs++;
    }

    // Read needed block from memory and write into cache
    // TODO: check if 2 * bytes required
    block->replace(tag);
    stats->bytes_transferred += (1 << size.B);
}
