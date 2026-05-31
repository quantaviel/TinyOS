#pragma once

#include <stdint-gcc.h>

struct idt_desc {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define IDT_PRESENT   (1 << 15)
#define IDT_RING0     (0 << 13)
#define IDT_RING3     (3 << 13)

#define IDT_SIZE32    (1 << 11)
#define IDT_INT_GATE  ((0b110) << 8)
#define IDT_TRAP_GATE ((0b111) << 8)
#define IDT_TASK_GATE ((0b101) << 8)