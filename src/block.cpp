#include "block.hpp"

#include <iostream>

Block::Block(u64 B, u64 K, bool sb) : B(B), K(K), sb(sb) {
    // Skip init if no subblocking
    if (!sb) return;
    
    // Number of subblocks = 2^B / 2^K
    n = (1 << (B-K));
    valid.resize(n, 0);
    std::fill(valid.begin(), valid.end(), 0);
}

// Read a single subblock
bool Block::read(u64 offset) {
    // No subblocking = always hit
    if (!sb) return true;

    int idx = find_idx(offset);
    
    if (idx > n)
        exit_on_error("Subblock index out of range.");

    if (valid[idx] == 1)
        return true;
    
    return false;
}

bool Block::write(u64 subblock) {
    if (!sb) return false;
    
    if (subblock > n)
        exit_on_error("Subblock index out of range.");

    // Only fetch invalid subblocks
    if (valid[subblock] == 0) {
        valid[subblock] = 1;
        return true;
    }

    return false;
}

// Write multiple subblocks (prefetch)
// Returns number of blocks written
int Block::write_many(u64 offset) {
    // If no subblocking, always write entire block
    if (!sb) return (1 << B);
    
    // Figure out which subblock is being requested
    int idx = find_idx(offset);
    int c = 0;
    
    // Iterate through all offsets
    // Only count invalid subblock writes
    for (; idx < n; idx++)
        if (write(idx))
            c++;

    return c * (1 << K);
}

void Block::replace(u64 tag, bool full) {    
    this->tag = tag;
    this->dirty = false;

    // Full block replace => all valid
    if (full)
        std::fill(valid.begin(), valid.end(), 1);
}

int Block::num_valid() {
    // Returns number of valid subblocks
    int c = 0;

    for (int i = 0; i < n; i++)
        if (valid[i] == 1)
            c++;
    
    return c;
}

int Block::num_invalid(u64 offset) {
    int idx = find_idx(offset);
    int c = 0;

    for (; idx < n; idx++)
        if (valid[idx] == 0)
            c++;
    
    return c;
}

int Block::find_idx(u64 offset) {
     // Figure out which subblock is being requested
    float max_offset = (1 << B) - 1;
    float ratio = offset / max_offset;
    int idx = static_cast<int>(ratio * (n-1));
    return idx;
}