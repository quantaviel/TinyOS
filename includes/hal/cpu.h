#pragma once

#include <stdint-gcc.h>

namespace cpu {

typedef unsigned int reg32;
typedef unsigned short reg16;

struct gp_regs {
    reg32 eax;
    reg32 ebx;
    reg32 ecx;
    reg32 edx;
    reg32 edi;
    reg32 ebp;
    reg32 esi;
    reg32 esp;
}__attribute__((packed));

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile (
        "outb %0, %1"
        :
        : "a"(val), "Nd"(port)
    );
}

static inline auto inb(uint16_t port) -> uint8_t {
    uint8_t ret;
    asm volatile (
        "inb %w1, %b0" 
        : "=a"(ret) 
        : "Nd"(port) 
        : "memory"
    );
    return ret;
}

void pic_disable();
void tlb_refresh(uintptr_t addr);

void disable_interrupts();
void enable_interrupts();
void read_msr(uint32_t msr, uint32_t* low, uint32_t* high);
void write_msr(uint32_t msr, uint32_t low, uint32_t high);

} // namespace cpu