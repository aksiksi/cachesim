// C++ includes
#include <iostream>

#include "cache.hpp"

Cache::Cache(CacheSize size, CacheType ct, cache_stats_t* cs, bool vc) : 
            size(size), ct(ct), stats(cs), vc(vc) {
    u64 C = size.C, B = size.B, S = size.S, K = size.K, V = size.V;
    
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
    int max_size;

    if (ct == FULLY_ASSOC)
        max_size = rows;
    else if (ct == SET_ASSOC)
        max_size = cols;

    if (ct != DIRECT_MAPPED) {
        for (int i = 0; i < rows; i++)
            lru.push_back(std::make_shared<LRU>(max_size));
    }

    // Init VC
    if (this->vc)
        victim_cache = new VictimCache(V);

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

Cache::~Cache() {
    // Free up VC
    if (this->vc)
        delete victim_cache;
}

std::shared_ptr<Block>
Cache::find_block(const u64 tag, const u64 index) {
    std::shared_ptr<Block> block;

    if (ct == FULLY_ASSOC) {
        // FA cache -> search for tag directly
        for (auto& each: cache) {
            auto b = each[0];

            // Check for tag
            if (b->tag == tag) {
                block = b;
                break;
            }
        }
    } else if (ct == DIRECT_MAPPED) {
        // Retrieve the "only" possible block
        block = cache[index][0];
    } else {
        // Set associative cache
        // Retrieve set based on index
        auto& set = cache[index];

        for (auto b: set) {
            if (b->tag == tag) {
                block = b;
                break;
            }
        }
    }

    return block;
}

std::shared_ptr<Block>
Cache::check_vc(const u64 addr) {
    const u64 tag = get_tag(addr);
    const u64 index = get_index(addr);
    const u64 offset = get_offset(addr);

    std::shared_ptr<Block> block;

    // Find the next victim
    // tag = 0 if empty block
    auto victim = find_victim(index);

    // Check VC, if applicable
    // Check VC even if block is empty (???)
    if (vc) {
        int pos = victim_cache->lookup(tag, index);

        // Target block is a hit in VC
        if (pos != -1) {
            // Remove target from VC, return ptr to it
            Block *target = victim_cache->remove(pos);

            // Perform eviction and copy block back to cache
            block = evict(tag, index);
            block.reset(target);

            auto p1 = &victim;
            auto p2 = &block;
            auto p3 = &this->cache[51][7];

            // Check for subblock miss
            if (!block->read(offset)) {
                stats->bytes_transferred += block->num_invalid(offset);;
                block->write_many(offset);
                stats->subblock_misses++;
            }
        } else {
            // VC miss
            stats->vc_misses++;
        }
    }

    return block;
}

CacheResult Cache::read(u64 addr) {
    const u64 tag = get_tag(addr);
    const u64 index = get_index(addr);
    const u64 offset = get_offset(addr);

    // Add access to LRU
    lru_push(tag, index);

    stats->accesses++;
    stats->reads++;

    auto block = find_block(tag, index);
    bool hit = false;

    if (block != nullptr)
        hit = true;

    CacheResult cr;

    /// TODO:
    // SB MISSES ARE OFF
    // THIS MEANS THAT ISSUES WITH VALID BITS??
    // PERHAPS EDGE CASE IN READ/WRITE_MANY

    if (hit) {
        // Subblock hit
        if (block->read(offset))
            cr = READ_HIT;
        // Subblock miss! -> Perform prefetch
        else {
            // Only count invalid subblocks for prefetch
            stats->bytes_transferred += block->num_invalid(offset);
            block->write_many(offset);

            stats->subblock_misses++;
        
            cr = READ_SB_MISS;
        }
    } else {
        // Full read miss
        stats->read_misses++;

        // Check the VC first
        // If hit, handle it within check_vc
        block = check_vc(addr);

        // VC miss
        if (block == nullptr) {
            // Find suitable victim to evict
            // Or return first empty block
            // Note: *only* if not already found
            block = evict(tag, index);

            // Retrieve subblock and prefetch subsequent
            // Fetch required subblocks from memory
            int bytes = block->write_many(offset);
            stats->bytes_transferred += bytes;

            if (vc) {
                // Missed both cache and VC
                stats->read_misses_combined++;
            }
        }

        cr = READ_MISS;
    }

    return cr;
}

CacheResult Cache::write(u64 addr) {
    const u64 tag = get_tag(addr);
    const u64 index = get_index(addr);
    const u64 offset = get_offset(addr);

    lru_push(tag, index);

    stats->accesses++;
    stats->writes++;

    // Find block in cache
    // If not present, = nullptr
    auto block = find_block(tag, index);
    bool hit = false;

    if (block != nullptr)
        hit = true;

    CacheResult cr;

    if (hit) {
        if (block->read(offset)) {
            cr = WRITE_HIT;
        } else {
            // Subblock miss
            stats->subblock_misses++;

            cr = WRITE_SB_MISS;
        }
    } else {
        // Full write miss
        stats->write_misses++;

        // Check the VC first
        block = check_vc(addr);

        if (block == nullptr) {
            // Find suitable victim to evict
            // Or return first empty block
            // Note: *only* if not already found
            block = evict(tag, index);

            if (vc) {
                // Missed both cache and VC
                stats->write_misses_combined++;
            }
        }

        cr = WRITE_MISS;
    }

    // Write invalid subblocks needed into block in cache
    stats->bytes_transferred += block->num_invalid(offset);
    block->write_many(offset);

    // Always set as dirty
    block->dirty = true;

    return cr;
}

std::shared_ptr<Block>
Cache::evict(u64 tag, u64 index) {
    // Find a block to evict from cache (victim)
    // Returns tag = 0 if empty slot found
    auto block = find_victim(index);

    // If empty block, just ignore the eviction
    // If VC active, do not writeback now!
    if (block->tag != 0 && !vc) {
        // Replace the block in cache
        // Check if dirty first => writeback
        if (block->dirty) {
            // Write back valid subblocks to memory
            int num_valid = block->num_valid();
            stats->bytes_transferred += num_valid;
            stats->write_backs++;
        }
    } else if (block->tag != 0 && vc) {
        // If VC active, push evicted block to VC
        victim_cache->push(block, stats);
    }

    block->replace(tag, index, false);

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
        auto& l = lru[index];
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
