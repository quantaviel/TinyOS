#pragma once

#include <stdint-gcc.h>

#define IOAPIC_BASE_VADDR       0x2000

#define IOAPIC_IOREGSEL         *((uint32_t *)(IOAPIC_BASE_VADDR))
#define IOAPIC_IOWIN            *((uint32_t *)(IOAPIC_BASE_VADDR + 0x10))

#define IOAPIC_ID               0
#define IOAPIC_VER              1
#define IOAPIC_ARB              2

#define IOAPIC_IOREDTBL_LOW(i)  (0x10 + (i) * 2)
#define IOAPIC_IOREDTBL_HIG(i)  (0x10 + (i) * 2 + 1)

#define IOAPIC_DELMOD_FIXED     0b000
#define IOAPIC_DELMOD_LPRIORITY 0b001
#define IOAPIC_DELMOD_SMI       0b010
#define IOAPIC_DELMOD_NMI       0b100
#define IOAPIC_DELMOD_INIT      0b101
#define IOAPIC_DELMOD_EXTINT    0b111

#define IOAPIC_DESTMOD_PHY      0
#define IOAPIC_DESTMOD_LOGICAL  1

#define IOAPIC_INTPOL_HIGH      0
#define IOAPIC_INTPOL_LOW       1

#define IOAPIC_TRIGMOD_EDGE     0
#define IOAPIC_TRIGMOD_LEVEL    1

#define IOAPIC_SET_DELMOD(x)    ((x) << 8)
#define IOAPIC_SET_DESTMOD(x)   ((x) << 11)
#define IOAPIC_SET_INTPOL(x)    ((x) << 13)
#define IOAPIC_SET_TRIGMOD(x)   ((x) << 15)
#define IOAPIC_SET_MASK         (1 << 16)

#define IOAPIC_SET_DESTADDR(x)  ((x) << (56 - 32))

namespace ioapic {

uint32_t read(uint32_t offset);
void write(uint32_t offset, uint32_t value);
void init();

} // namespace ioapic

