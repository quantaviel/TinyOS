#pragma once

#include <mm/vmm.h>

namespace mm {

class kheap {
public:
    void init(uintptr_t head, uintptr_t limit);
    uintptr_t kalloc(size_t size);
    void kfree(void *addr);
private:
    bool extend(size_t size);
    void merge(uintptr_t chunk_addr);
    uintptr_t head;
    uintptr_t end;
    uintptr_t limit;
};

} // namespace mm