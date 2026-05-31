#include <arch/x86/gdt.h>

constexpr int GDT_ENTRY_SIZE = 5;

uint64_t gdt[GDT_ENTRY_SIZE];
uint32_t gdt_limit = sizeof(gdt);

void set_gdt_entry(
    int index,
    uint32_t base,
    uint32_t limit,
    uint32_t sd_flags
) {
    gdt[index] = 0;
    gdt[index] = (base & 0xFF000000) | 
                 (limit & 0x000F0000) | 
                 ((base & 0x00FF0000) >> 16) |
                sd_flags;
    gdt[index] <<= 32;
    gdt[index] |= ((base & 0x0000FFFF) << 16) |
                  (limit & 0x0000FFFF);
}

extern "C"
void gdt_init() {
    set_gdt_entry(0, 0, 0, 0);
    set_gdt_entry(1, 0, 0xFFFFF, SD_R0_CODE);
    set_gdt_entry(2, 0, 0xFFFFF, SD_R0_DATA);
    set_gdt_entry(3, 0, 0xFFFFF, SD_R3_CODE);
    set_gdt_entry(4, 0, 0xFFFFF, SD_R3_DATA);

    gdtr_t gdtr;
    gdtr.limit = gdt_limit;
    gdtr.base = (uintptr_t)gdt;

    asm volatile (
        "lgdt %0" : : "m"(gdtr)
    );
}
