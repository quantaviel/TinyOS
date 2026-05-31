#pragma once

#define KPG_SIZE                    (1024 * 6 * 4)

#ifndef __ASM_FILE__

#include <stdint-gcc.h>

// Defined by linker.ld
extern uint32_t __prologue_end;
extern uint32_t __kernel_start;
extern uint32_t __kernel_end;
extern uint32_t __kernel_heap_start;
extern uint32_t _k_ptd;

#define sys_val(x)                  (uint32_t)(&(x))
#define PROLOGUE_END                (sys_val(__prologue_end))
#define KERNEL_START                (sys_val(__kernel_start))
#define KERNEL_END                  (sys_val(__kernel_end))
#define KERNEL_HEAP_START           (sys_val(__kernel_heap_start))

#define KERNEL_STACK_START          (0xFFBFFFFF)
#define KERNEL_STACK_SIZE           (1 * 1024 * 1024)

#define PAGE_SHIFT                  (12)
#define PAGE_SIZE                   (1 << PAGE_SHIFT)
#define WSIZE                       (4)

#define PROLOGUE_PAGE_COUNT         ((PROLOGUE_END + 0xFFF) >> 12)
#define KERNEL_PAGE_COUNT           ((KERNEL_END - KERNEL_START + 0xFFF) >> 12)

#define VIRTUAL_KERNEL_BEGIN        0xC0000000
#define V2P(x)                      ((uintptr_t)(x) - VIRTUAL_KERNEL_BEGIN)
#define P2V(x)                      ((uintptr_t)(x) + VIRTUAL_KERNEL_BEGIN)
#define KERNEL_PTD_PADDR            (V2P(sys_val(_k_ptd)))

/* defines about page directory & page table */

#define PDE_INDEX(phy_addr)         (((phy_addr) & 0xFFC00000) >> 22)
#define PTE_INDEX(phy_addr)         (((phy_addr) & 0x003FF000) >> 12)
#define PTE_COUNT 1024

typedef uint32_t ptd_t;
typedef uint32_t pt_t;
typedef uint32_t pde_t;
typedef uint32_t pte_t;
typedef uint32_t pte_attr_t;
typedef uint32_t pde_attr_t;

/**
 * PTE is Page Table Entry, which maps a 4-KByte page.
 * PTE Attributes
 */

// Present; must be 1 to map a 4-KByte page
#define PTE_PRESENT         (1 << 0)
// Read/write; if 0, writes may not be allowed to the 4-KByte page referenced by this entry (see Section 4.6)
#define PTE_READ_WRITE      (1 << 1)
// User/supervisor; if 0, user-mode accesses are not allowed to the 4-KByte page referenced by this entry (see Section 4.6)
#define PTE_USER_AVAL       (1 << 2)
// Page-level write-through; indirectly determines the memory type used to access the 4-KByte page referenced by this entry (see Section 4.9)
#define PTE_WRITE_THROUGH   (1 << 3)
// Page-level cache disable; indirectly determines the memory type used to access the 4-KByte page referenced by this entry (see Section 4.9)
#define PTE_DISABLE_CACHE   (1 << 4)
// If the PAT is supported, indirectly determines the memory type used to access the 4-KByte page referenced by this entry (see Section 4.9.2); otherwise, reserved (must be 0)
#define PTE_PAT_ENABLE      (1 << 7)
// Global; if CR4.PGE = 1, determines whether the translation is global (see Section 4.10); ignored otherwise
#define PTE_GLOBAL          (1 << 8)

/**
 * PDE is Page Directory Entry, which references a page table.
 * PDE Attrributes
 */

// Present; must be 1 to map a 4-KByte page
#define PDE_PRESENT         (1 << 0)
// Read/write; if 0, writes may not be allowed to the 4-KByte page referenced by this entry (see Section 4.6)
#define PDE_READ_WRITE      (1 << 1)
// User/supervisor; if 0, user-mode accesses are not allowed to the 4-KByte page referenced by this entry (see Section 4.6)
#define PDE_USER_AVAL       (1 << 2)
// Page-level write-through; indirectly determines the memory type used to access the 4-KByte page referenced by this entry (see Section 4.9)
#define PDE_WRITE_THROUGH   (1 << 3)
// Page-level cache disable; indirectly determines the memory type used to access the 4-KByte page referenced by this entry (see Section 4.9)
#define PDE_DISABLE_CACHE   (1 << 4)
// If CR4.PSE = 1, must be 0 (otherwise, this entry maps a 4-MByte page; see Table 4-4); otherwise, ignored
#define PDE_4MB_PAGE        (1 << 7)

// For kernel page directory
#define PDE_ATTR            (PDE_READ_WRITE)
// For kernel page table
#define PTE_ATTR            (PTE_READ_WRITE)


// Construct PTE & PDE
#define PTE(pte_attr, phy_addr)     ((pte_attr) | (phy_addr) | PTE_PRESENT)
#define PDE(pde_attr, pt_addr)      ((pde_attr) | (pt_addr) | PDE_PRESENT)

#define set_pd(pd, pde, value) pd[pde] = value
#define set_pt(pt, pte, value) pt[pte] = value


#endif