// C++ includes
#include <iostream>

#include "cachesim.hpp"
#include "cache.hpp"

// Init subblocks
Block::Block(u64 B, u64 K, bool sb) : B(B), sb(sb) {
    if (!sb) return;
    
    // Number of subblocks = 2^B / 2^K
    n = (1 << (B-K));
    subblocks.reserve(n);
    std::fill(subblocks.begin(), subblocks.end(), 0);
}

// Read a single subblock
bool Block::read(u64 offset) {
    // No subblocking = always hit
    if (!sb) return true;
    
    // Figure out which subblock is being requested
    int idx = offset % n;
    if (idx > n)
        exit_on_error("Subblock index out of range.");

    if (subblocks[idx] == 1)
        return true;
    
    return false;
}

void Block::write(u64 offset) {
    if (!sb) return;
    
    int idx = offset % n;
    if (idx > n)
        exit_on_error("Subblock index out of range.");

    subblocks[idx] = 1;
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
    std::fill(subblocks.begin(), subblocks.end(), 0);
    this->tag = tag;
    this->valid = 1;
}

Cache::Cache(CacheSize size, CacheType ct, cache_stats_t* cs) : 
            size(size), ct(ct), stats(cs) {
    u64 C = size.C, B = size.B, S = size.S, K = size.K;
    
    offset_mask = (1 << B) - 1;
    std::cout << std::hex << offset_mask << std::endl;

    if (ct == CacheType::DIRECT_MAPPED || ct == CacheType::SET_ASSOC)
        index_mask = ((1 << (C-B-S)) - 1) << B;

    std::cout << std::hex << index_mask << std::endl;

    tag_mask = ~(index_mask | offset_mask);
    std::cout << std::hex << tag_mask << std::dec << std::endl;

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
}

bool Cache::read(u64 addr) {
    u64 tag = get_tag(addr);
    u64 index = get_index(addr);
    u64 offset = get_offset(addr);

    stats->accesses++;
    stats->reads++;

    if (ct == CacheType::FULLY_ASSOC) {
        // FA cache -> search for tag directly
        for (auto each: cache) {
            auto block = each[0];

            // Check for tag in cache
            if (block->tag == tag) {
                // Check for subblock
                if (block->read(offset))
                    return true;
                // Subblock miss! -> Perform prefetch
                else {
                    // Retrieve current and all following from memory
                    stats->subblock_misses++;
                    int bytes = block->write_many(offset);
                    stats->bytes_transferred += bytes;
                }
            }
        }

        // Block not found, retrieve from memory and store in cache
        stats->read_misses++;

        // Retrieve block and write to cache
        auto block = cache[0][0]; // TODO: better replace (global LRU)
        block->replace(tag);
        stats->bytes_transferred += (1 << size.B);
    } else if (ct == CacheType::DIRECT_MAPPED) {
        // Retrieve the "only" possible block
        auto block = cache[index][0];

        if (block->tag == tag)
            return true;
        else {
            // Block miss!
            stats->read_misses++;
            block->replace(tag);
            stats->bytes_transferred += (1 << size.B);
        }
    } else {
        // Set associative cache
        // Retrieve set based on index
        std::vector<std::shared_ptr<Block>> set = cache[index];

        for (auto block: set) {
            if (block->tag == tag)
                // TODO: subblocking?
                return true;
        }

        // Block not found, retrieve from memory and store in cache
        stats->read_misses++;

        // Retrieve block and write to cache
        auto block = set[0]; // TODO: better replace (based on LRU)
        block->replace(tag);
        stats->bytes_transferred += (1 << size.B);
    }

    return false;
}

 bool Cache::write(u64 addr) {
    // TODO: incorporate replacement
    // Replacement differs:
    // DM: MUST place at tag position
    // FA: can place anywhere (use LRU stack)
    // SA: MUST place in correct set (use index mask)
    bool hit = false;

    return hit;
}
