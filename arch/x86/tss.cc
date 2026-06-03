#include <arch/x86/tss.h>
#include <arch/x86/gdt.h>

#include <klib/string.h>

tss global_tss;

void init_tss() {
    memset(&global_tss, 0, sizeof(global_tss));
    global_tss.ss0 = SEGMENT_R0_DATA;
    global_tss.esp0 = 0xFFBFFFFF;
    global_tss.iobase = sizeof(global_tss);

    // set tss 0x28 (5 * 8)
    asm volatile ("ltr %%ax" :: "a"(0x28));
}