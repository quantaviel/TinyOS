#pragma once

#include <mm/memory.h>
#include <mm/page.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

namespace mm {

namespace pt {

// About Page Table
bool map(vaddr_t vaddr, paddr_t paddr, uint32_t pde_flags, uint32_t pte_flags);
bool map_addrs(vaddr_t vaddr, paddr_t paddr, size_t size, uint32_t pde_flags, uint32_t pte_flags);
bool unmap(vaddr_t vaddr);
paddr_t translate(vaddr_t vaddr);

bool map_to_page_dir(vaddr_t page_dir, vaddr_t vaddr, paddr_t paddr, uint32_t pde_flags, uint32_t pte_flags);

paddr_t translate(vaddr_t page_dir, vaddr_t vaddr);

vaddr_t get_temp_page();

} // namespace pt

namespace vmm {

void bitmap_init(vaddr_t start, vaddr_t end);
/* Find a avaliable page in virtual space */
vaddr_t alloc_bit_page();
void free_bit_page(vaddr_t vaddr);
void mark_bit_page(vaddr_t vaddr);
void unmark_bit_page(vaddr_t vaddr);

/* Find a avaliable page in physical space and map it */
paddr_t alloc_page(vaddr_t vaddr);
paddr_t alloc_pages(vaddr_t vaddr, size_t len);
void    free_page(vaddr_t vaddr);
void    free_pages(vaddr_t vaddr, size_t len);

}

} // namespace mm