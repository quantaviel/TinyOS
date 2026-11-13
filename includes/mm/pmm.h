#pragma once

#include <arch/x86/multiboot.h>
#include <mm/memory.h>

#include <stddef.h>

namespace mm {

class pmm {
public:
    static void init(multiboot_info_t* mbi);

    static paddr_t alloc_page();
    static void    free_page(paddr_t paddr);

    static void mark_page(paddr_t paddr);
    static void unmark_page(paddr_t paddr);

    static void mark_pages(paddr_t paddr, size_t len);
    static void unmark_pages(paddr_t paddr, size_t len);
    
    static int get_refcount(paddr_t paddr);
    static void inc_refcount(paddr_t paddr);
    static void dec_refcount(paddr_t paddr);
private:

    static bool is_marked(paddr_t paddr);

    static constexpr unsigned BITMAP_SIZE = PAGE_MAX >> 3;
    static uint8_t bitmap[BITMAP_SIZE];
    static uint8_t refcount[PAGE_MAX];
};

} // namespace mm