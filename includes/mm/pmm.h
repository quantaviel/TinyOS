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
private:

    static bool is_marked(paddr_t paddr);

    static constexpr unsigned BITMAP_SIZE = PAGE_MAX >> 3;
    static uint8_t bitmap[BITMAP_SIZE];
};

} // namespace mm