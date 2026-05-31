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

} // namespace pt

namespace vmm {

paddr_t alloc_page(vaddr_t vaddr);
paddr_t alloc_pages(vaddr_t vaddr, size_t len);
void    free_page(vaddr_t vaddr);
void    free_pages(vaddr_t vaddr, size_t len);

}

} // namespace mm