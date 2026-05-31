#pragma once

#include <mm/vmm.h>
#include <mm/heap.h>

namespace mm {

constexpr unsigned long kheap_size = 0x100000;

class kvm {
public:
    static kheap& get_kheap() {
        static kheap kernel_heap;
        return kernel_heap;
    }
    static void init(uintptr_t head, uintptr_t limit) {
        get_kheap().init(head, limit);
    }
    static uintptr_t kalloc(size_t size) {
        return get_kheap().kalloc(size);
    }
    static void kfree(void *addr) {
        return get_kheap().kfree(addr);
    }
};

} // namespace mm