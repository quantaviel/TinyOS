#pragma once

#include <stdint-gcc.h>
#include <stddef.h>

/**
 * Circular Queue - Thread-UnSafe Ring Buffer
 * Used for keyboard interrupt buffer
 */
template<typename T, size_t Capacity>
class circular_queue {
public:
    circular_queue() : head_(0), tail_(0), count_(0), ALLOW_OVERFLOW(false) {}
    
    explicit
    circular_queue(bool overflow) : head_(0), tail_(0), count_(0), ALLOW_OVERFLOW(overflow) {}

    // Push an element to the tail (producer side)
    bool push(T value) {
        if (count_ >= Capacity) {
            if (ALLOW_OVERFLOW) {
                head_ = (head_ + 1) % Capacity;
                count_--;
            } else {
                return false;  // Buffer full
            }
        }
        buffer_[tail_] = value;
        tail_ = (tail_ + 1) % Capacity;
        count_++;
        return true;
    }

    // Pop an element from the head (consumer side)
    bool pop(T& out) {
        if (count_ == 0) {
            return false;  // Buffer empty
        }
        out = buffer_[head_];
        head_ = (head_ + 1) % Capacity;
        count_--;
        return true;
    }

    // Peek at the front element without removing it
    bool peek(T& out) const {
        if (count_ == 0) {
            return false;
        }
        out = buffer_[head_];
        return true;
    }

    // Check if queue is empty
    bool empty() const { return count_ == 0; }

    // Check if queue is full
    bool full() const { return count_ >= Capacity; }

    // Get current number of elements
    size_t size() const { return count_; }

    // Get remaining capacity
    size_t remaining() const { return Capacity - count_; }

    // Clear all elements
    void clear() { head_ = tail_ = count_ = 0; }

private:
    T buffer_[Capacity];
    size_t head_;
    size_t tail_;
    size_t count_;

    const bool ALLOW_OVERFLOW;
};