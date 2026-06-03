#include <mm/memory.h>
#include <mm/page.h>
#include <mm/pmm.h>

#include <vga.h>

#include <stdint-gcc.h>

namespace mm {

uint8_t pmm::bitmap[pmm::BITMAP_SIZE] = {0};
uint8_t pmm::refcount[PAGE_MAX] = {0};

void pmm::init(multiboot_info_t *mbi) {
    for (int i = 0; i < BITMAP_SIZE; ++i) bitmap[i] = 0xFF;
    for (int i = 0; i < PAGE_MAX; ++i) refcount[i] = 0;
}

void pmm::mark_page(paddr_t paddr) {
    size_t page_number = paddr >> PAGE_SHIFT;
    bitmap[page_number >> 3] |= (1 << (page_number & 7));
}

void pmm::unmark_page(paddr_t paddr) {
    size_t page_number = paddr >> PAGE_SHIFT;
    bitmap[page_number >> 3] &= ~(1 << (page_number & 7));
}

void pmm::mark_pages(paddr_t paddr, size_t len) {
    if (len == 0) return;
    size_t first = paddr >> PAGE_SHIFT;
    size_t last  = (paddr + len - 1) >> PAGE_SHIFT;
    for (size_t pn = first; pn <= last; ++pn) {
        size_t byte = pn >> 3;
        uint8_t bit  = pn & 7;
        bitmap[byte] |= (1 << bit);
    }
    /// FUTURE: need to mark 8 bit in one operation
}

void pmm::unmark_pages(paddr_t paddr, size_t len) {
    if (len == 0) return;
    size_t first = paddr >> PAGE_SHIFT;
    size_t last  = (paddr + len - 1) >> PAGE_SHIFT;
    for (size_t pn = first; pn <= last; ++pn) {
        size_t byte = pn >> 3;
        uint8_t bit  = pn & 7;
        bitmap[byte] &= ~(1 << bit);
    }
    /// FUTURE: need to mark 8 bit in one operation
}

paddr_t pmm::alloc_page() {
    for (size_t i = 0; i < BITMAP_SIZE; ++i) {
        if (bitmap[i] == 0xFF) continue;
        uint8_t free_mask = (~bitmap[i]) & (bitmap[i] + 1);
        size_t bit = __builtin_ctz(free_mask);
        size_t page = (i << 3) + bit;

        if (page == 0) continue;
        
        bitmap[i] |= (1 << bit);
        inc_refcount(page << PAGE_SHIFT);
        return page << PAGE_SHIFT;
    }
    return 0;
    /// FUTURE: change to next-fit algorithm
}

void pmm::free_page(paddr_t addr) {
    if (!is_marked(addr)) return;
    unmark_page(addr);
    dec_refcount(addr);
}

bool pmm::is_marked(paddr_t paddr) {
    return bitmap[paddr >> 15] & (1 << ((paddr >> 12) & 7));
}

auto pmm::get_refcount(paddr_t paddr) -> int {
    return refcount[paddr >> PAGE_SHIFT];
}

void pmm::inc_refcount(paddr_t paddr) {
    refcount[paddr >> PAGE_SHIFT] ++;
}

void pmm::dec_refcount(paddr_t paddr) {
    uint32_t page_number = paddr >> PAGE_SHIFT;
    if (refcount[page_number] > 0) refcount[page_number]--;
}


} // namespace mm
