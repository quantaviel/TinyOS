#include <hal/cpu.h>
#include <mm/vmm.h>

#include <klib/fmt.h>

namespace mm {

namespace pt {

// About Page Table Map & Unmap

#define PD_VADDR 0xFFFFF000;
#define PT_VADDR(i) (0xFFC00000 | ((i) << PAGE_SHIFT))

#define PAGE_ALIGN(addr) ((addr) & 0xFFFFF000)
#define PT_INDEX(addr) ((addr) >> 12 & 0x3FF)
#define PD_INDEX(addr) ((addr) >> 22 & 0x3FF)

bool map(vaddr_t va, paddr_t pa, uint32_t pde_flags, uint32_t pte_flags) {
    if (!va || !pa) return false;
    if (va >= 0xFFC00000) return true;
    va = PAGE_ALIGN(va);
    pa = PAGE_ALIGN(pa);
    pde_t* pdv = (pde_t*)PD_VADDR;
    size_t pdi = PD_INDEX(va);
    if (!(pdv[pdi] & 1)) { // page table doesn't present
        paddr_t new_pt = mm::pmm::alloc_page();
        if (!new_pt) return false;
        pdv[pdi] = PDE(new_pt, pde_flags);

        pte_t* ptv = (pte_t*)PT_VADDR(pdi);
        for (size_t i = 0; i < PTE_COUNT; ++i)
            ptv[i] = 0;
    }
    volatile pte_t* ptv = (pte_t*)PT_VADDR(pdi);
    size_t pti = PT_INDEX(va);

    ptv[pti] = PTE(pa, pte_flags);

    return true;
}

bool unmap(vaddr_t va) {
    if (!va) return false;
    pde_t* pdv = (pde_t*)PD_VADDR;
    size_t pdi = PD_INDEX(va);
    if (!(pdv[pdi] & 1)) return false;

    pte_t* ptv = (pte_t*)PT_VADDR(pdi);
    size_t pti = PT_INDEX(va);

    if (!(ptv[pti] & 1)) return false;
    ptv[pti] = 0;
    if (!(ptv[pti] & PTE_DISABLE_CACHE)) cpu::tlb_refresh(va);
    return true;
}

paddr_t translate(vaddr_t va) {
    if (!va) return 0;
    pde_t* pdv = (pde_t*)PD_VADDR;
    size_t pdi = PD_INDEX(va);
    if (!(pdv[pdi] & 1)) return 0;
    
    pte_t* ptv = (pte_t*)PT_VADDR(pdi);
    size_t pti = PT_INDEX(va);
    if (!(ptv[pti] & 1)) return 0;
    return (ptv[pti] & 0xFFFFF000) | (va & 0xFFF);
}

bool map_addrs(vaddr_t vaddr, paddr_t paddr, size_t size, uint32_t pde_flags, uint32_t pte_flags) {
    uint32_t page_count = size >> PAGE_SHIFT;
    for (int i = 0; i < page_count; ++i) {
        auto res = map(vaddr + i * PAGE_SIZE, paddr + i * PAGE_SIZE, pde_flags, pte_flags);
        if (!res) {
            // fmt::println("Error on map {x} to {x}", vaddr+i*PAGE_SIZE, paddr+i*PAGE_SIZE);
            // return false;
        }
    }
    return true;
}

} // namespace pt

namespace vmm {

paddr_t alloc_page(vaddr_t vaddr) {
    paddr_t paddr = mm::pmm::alloc_page();
    if (!paddr) return 0;
    if (!pt::map(vaddr, paddr, PDE_ATTR, PTE_ATTR)) {
        mm::pmm::free_page(paddr);
        return 0;
    }
    return paddr;
}

paddr_t alloc_pages(vaddr_t vaddr, size_t len) {
    if (len == 0) return 0;
    vaddr = PAGE_ALIGN(vaddr);
    size_t page_count = (len + 0xFFF) >> 12;
    paddr_t ppages[4096] = {};
    for (size_t i = 0; i < page_count; ++i) {
        paddr_t paddr = mm::pmm::alloc_page();
        ppages[i] = paddr;
        if (!paddr) {
            // free already allocated pages
            for (size_t j = 0; j <= i; ++j) {
                pt::unmap(vaddr + (j << PAGE_SHIFT));
                mm::pmm::free_page(ppages[j]);
            }
            return 0;
        }
        if (!pt::map(vaddr + (i << PAGE_SHIFT), paddr, PDE_ATTR, PTE_ATTR)) {
            mm::pmm::free_page(paddr);
            // free already allocated pages
            for (size_t j = 0; j <= i; ++j) {
                pt::unmap(vaddr + (j << PAGE_SHIFT));
                mm::pmm::free_page(ppages[j]);
            }
            return 0;
        }
    }
    return ppages[0];
}

void free_page(vaddr_t vaddr) {
    paddr_t paddr = pt::translate(vaddr);
    if (!paddr) return;
    pt::unmap(vaddr);
    mm::pmm::free_page(paddr);
}

void free_pages(vaddr_t vaddr, size_t len) {
    if (len == 0) return;
    size_t page_count = (len + 0xFFF) >> 12;
    for (size_t i = 0; i < page_count; ++i) {
        paddr_t paddr = pt::translate(vaddr + (i << 12));
        if (!paddr) continue;
        pt::unmap(vaddr + (i << 12));
        mm::pmm::free_page(paddr);
    }
}

} // namspace vmm

} // namespace mm