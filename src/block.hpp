#ifndef BLOCK_H
#define BLOCK_H

#include <vector>

#include "cachesim.hpp"

/**
    Represents a single block in a cache.
*/
class Block {
public:
    // Stores valid bits for each subblock
    std::vector<int> valid;
    u64 tag = 0, index = 0;
    bool dirty = false;
    
    int n; // Number of subblocks
    u64 B; // Block size
    u64 K; // Number of bytes per subblock

    bool sb; // Enable/disable subblocking

    Block(u64 B, u64 K, bool sb);

    // Copy constructor
    Block(const Block& other);
    Block& operator=(const Block& other);

    // Read a single subblock
    bool read(u64 offset);
    
    // Write a single sublock
    bool write(int subblock);

    // Write multiple subblocks (prefetch)
    int write_many(u64 offset);

    void replace(u64 tag, u64 index, bool full);

    // Used for writeback
    int num_valid();

    // Count num invalid starting from offset
    int num_invalid(u64 offset);

    // Find subblock idx given an offset
    int find_idx(u64 offset);
};

#endif
