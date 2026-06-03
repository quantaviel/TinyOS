#include <hal/cpu.h>

namespace cpu {

void pic_disable() {
    outb(0x21, 0xFF);  // 主 PIC，屏蔽 IRQ0–7
    outb(0xA1, 0xFF);  // 从 PIC，屏蔽 IRQ8–15
}

void tlb_refresh(uintptr_t addr) {
    asm volatile (
        "invlpg (%0)"
        :
        : "r"(addr)
        : "memory"
    );
}

void read_msr(uint32_t msr, uint32_t* low, uint32_t* high) {
    uint32_t edx, eax;
    asm volatile (
        "rdmsr"
        : "=d"(edx), "=a"(eax)
        : "c"(msr)
    );
    *low = eax;
    *high = edx;
}
void write_msr(uint32_t msr, uint32_t low, uint32_t high) {
    asm volatile (
        "wrmsr"
        :
        : "c"(msr), "a"(low), "d"(high)
        : "memory"
    );
}

void disable_interrupts() {
    asm volatile ("cli" ::: "memory");
}

void enable_interrupts() {
    asm volatile ("sti" ::: "memory");
}

void switch_cr3(uint32_t page_dir_addr) {
    page_dir_addr &= 0xFFFFF000;
    asm volatile (
        "movl %0, %%cr3"
        :
        : "r"(page_dir_addr)
        : "memory"
    );
}

auto get_esp() -> uint32_t {
    uint32_t esp;
    asm volatile ("movl %%esp, %0" : "=r"(esp));
    return esp;
}

} // namespace cpu