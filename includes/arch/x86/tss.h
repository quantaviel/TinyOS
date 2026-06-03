#pragma once

#include <stdint-gcc.h>

struct tss {
    uint32_t link; // Previous Task Link Field. Contains the Segment Selector for the TSS of the previous task.
    uint32_t esp0; // esp0, esp1, esp2: The Stack Pointers used to load the stack when a privilege level change occurs from a lower privilege level to a higher one.
    uint32_t ss0; // ss0, ss1, ss2: The Segment Selectors used to load the stack when a privilege level change occurs from a lower privilege level to a higher one.
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax; uint32_t ecx; uint32_t edx; uint32_t ebx;
    uint32_t esp; uint32_t ebp;
    uint32_t esi; uint32_t edi;
    uint32_t es; uint32_t cs; uint32_t ss; uint32_t ds;
    uint32_t fs; uint32_t gs;
    uint32_t ldtr;
    uint16_t trap;
    uint16_t iobase; // I/O Map Base Address Field. Contains a 16-bit offset from the base of the TSS to the I/O Permission Bit Map.
    uint32_t ssp; // Shadow Stack Pointer.
} __attribute__((packed));

void init_tss();