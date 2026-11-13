#pragma once

#include <mm/memory.h>

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

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile (
        "inb %w1, %b0" 
        : "=a"(ret) 
        : "Nd"(port) 
        : "memory"
    );
    return ret;
}

// 16位多字节搬运指令：从端口连续读入 data_count 个字 (Word) 到 buffer
static inline void insw(uint16_t port, void* addr, uint32_t cnt) {
    asm volatile("cld; rep insw" : "+D"(addr), "+c"(cnt) : "d"(port) : "memory");
}

// 16位多字节搬运指令：将 buffer 中 data_count 个字写入端口
static inline void outsw(uint16_t port, const void* addr, uint32_t cnt) {
    asm volatile("cld; rep outsw" : "+S"(addr), "+c"(cnt) : "d"(port) : "memory");
}

void pic_disable();
void refresh_tlb(mm::vaddr_t addr);
void refresh_all_tlb();

void disable_interrupts();
void enable_interrupts();
void read_msr(uint32_t msr, uint32_t* low, uint32_t* high);
void write_msr(uint32_t msr, uint32_t low, uint32_t high);

void switch_page_dir(mm::paddr_t page_dir_addr);
uint32_t get_esp();
mm::paddr_t get_page_dir();
void set_page_dir(mm::paddr_t addr);

void halt();

} // namespace cpu