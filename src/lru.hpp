#ifndef CACHESIM_LRU_H
#define CACHESIM_LRU_H

#include <forward_list>

#include "cachesim.hpp"

class LRU {
public:
    LRU(int m) : max_size(m) {}
    void push(u64 tag);
    u64 pop();
private:
    // Using forward_list for efficiency
    std::forward_list<u64> stack;
    size_t size = 0; // Current size of LRU
    int max_size; // Max LRU size

    // Stores the last popped value for case
    // of eviction push before pop!
    u64 last_popped = 0;
};

#endif //CACHESIM_LRU_HPP_H
