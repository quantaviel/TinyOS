#pragma once

#include <mm/memory.h>

#include <mm/vmm.h>

namespace mm {

class kheap {
public:
    void init(mm::vaddr_t head, mm::vaddr_t limit);
    mm::vaddr_t kalloc(size_t size);
    void kfree(void *addr);
private:
    bool extend(size_t size);
    void merge(mm::vaddr_t chunk_addr);
    mm::vaddr_t head;
    mm::vaddr_t end;
    mm::vaddr_t limit;
};

} // namespace mm