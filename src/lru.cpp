#include "lru.hpp"

void LRU::push(u64 tag) {
    bool found = false;
    int idx = 0;

    for (auto t: stack) {
        if (t == tag) {
            found = true;
            break;
        }

        idx++;
    }

    // Remove tag from stack
    // 1. Get an iterator to tag before target
    // 2. Then delete target tag
    if (found) {
        auto p = std::next(stack.before_begin(), idx);
        stack.erase_after(p);
        size--;
    }

    // Do a push
    stack.push_front(tag);
    size++;

    if (size > max_size) {
        // Iterator to before the last element
        auto it = std::next(stack.before_begin(), size-1);

        // Store ref to last popped
        last_popped = *std::next(it);

        // Erase the last element
        stack.erase_after(it);
        size--;
    }
}

u64 LRU::pop() {
    if (size != max_size) {
        // Pointer to before last element
        auto it = std::next(stack.before_begin(), size-1);

        // Get value of last element (pop)
        u64 tag = *std::next(it);

        // Remove last element
        stack.erase_after(it);
        size--;

        return tag;
    } else {
        // Just return the last popped value (from previous push)
        return last_popped;
    }
}

