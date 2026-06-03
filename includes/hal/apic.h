#pragma once

#include <stdint-gcc.h>

#define IA32_APIC_BASE_MSR                  0x1B
#define IA32_APIC_BASE_MSR_BSP              (1 << 8)  // Bit to indicate the bootstrap processor
#define IA32_APIC_BASE_MSR_ENABLE           (1 << 11) // Bit to enable the APIC

#define LAPIC_BASE_PADDR                    0xFEE00000
#define LAPIC_BASE_VADDR                    0xFEE00000

// Local APIC ID, different cpu has unique id
#define LAPIC_IDR                           0x20
// Local APIC Version
#define LAPIC_VER                           0x30
// Local APIC Task Priority Register
#define LAPIC_TPR                           0x80

// EOI is a register that should be writen
// someting after producing an interrupt
#define LAPIC_EOI                           0xB0

// Spurious Interrupt Vector Register
#define LAPIC_SVR                           0xF0

// Local Vector Table
#define LAPIC_LVT_TIMER                     0x320
#define LAPIC_LVT_LINT0                     0x350
#define LAPIC_LVT_LINT1                     0x360
#define LAPIC_LVT_ERROR                     0x370

// For timer
#define LAPIC_INIT_COUNT                    0x380
#define LAPIC_CURR_COUNT                    0x390
#define LAPIC_DIVIDE_CONF                   0x3E0

#define LAPIC_TIMER_DIVIDE_2                0b0000
#define LAPIC_TIMER_DIVIDE_4                0b0001
#define LAPIC_TIMER_DIVIDE_8                0b0010
#define LAPIC_TIMER_DIVIDE_16               0b0011
#define LAPIC_TIMER_DIVIDE_32               0b1000
#define LAPIC_TIMER_DIVIDE_64               0b1001
#define LAPIC_TIMER_DIVIDE_128              0b1010
#define LAPIC_TIMER_DIVIDE_1                0b1011

// Timer Register Option
#define LAPIC_TIMER_ONE_SHOT                (0b00 << 17)
#define LAPIC_TIMER_PERIODIC                (0b01 << 17)
#define LAPIC_TIMER_TSC_DDL                 (0b10 << 17)

// TPR priority class
#define LAPIC_TPR_CLASS(class)              ((class) << 4)
#define LAPIC_TPR_SUBCL(subcl)              ((subcl))

#define LAPIC_LVT_FIXED                     (0b000 << 8)
#define LAPIC_LVT_SMI                       (0b010 << 8)
#define LAPIC_LVT_NMI                       (0b100 << 8)
#define LAPIC_LVT_ExtINT                    (0b111 << 8)
#define LAPIC_LVT_INIT                      (0b101 << 8)

#define LAPIC_LVT_MASK                      (1 << 16)
#define LAPIC_LVT_NMASK                     (~(1 << 16))

#define LAPIC_LVT_TRIG_LEVEL                (1 << 15)
#define LAPIC_LVT_TRIG_EDGE                 ((0 << 15))

#define LAPIC_SVR_LAPIC_ENABLE              (1 << 8)

#define LAPIC_LVT_ERROR_VEC                 200
#define LAPIC_LVT_LINT0_VEC                 201
#define LAPIC_SVR_VEC                       202
#define LAPIC_LVT_TIMER_VEC                 203

namespace apic {

// Read Local APIC related register
static inline uint32_t read_reg(uint32_t offset) {
    volatile uint32_t* reg = (uint32_t*)(LAPIC_BASE_VADDR + offset);
    return *reg;
}

// Write Local APIC related register
static inline void write_reg(uint32_t offset, uint32_t value) {
    volatile uint32_t* reg = (uint32_t*)(LAPIC_BASE_VADDR + offset);
    *reg = value;
}

// Get Local APIC base address
uintptr_t get_base_addr();

// Set Local APIC base address
void set_base_addr(uintptr_t addr);

// Initialize Local APIC
void init();

// Enable Local APIC
void enable();

/*
Set APIC Timer with handler `func`
option: `LAPIC_TIMER_ONE_SHOT` or `LAPIC_TIMER_PERIODIC` or `LAPIC_TIMER_TSC_DDL`
divide: `LAPIC_TIMER_DIVIDE_1` to `LAPIC_TIMER_DIVIDE_128`
count: after the `count` times, `func` will be executed
*/
void set_apic_timer(uint32_t option, void (func)(int_frame_t*), uint32_t divide, uint32_t count);

// Get Local APIC ID
static inline int get_id() {
    return (read_reg(LAPIC_IDR) >> 24) & 0xFF;
}

// Get Local APIC Version
static inline int get_version() {
    return read_reg(LAPIC_VER) & 0xFF;
}

// Send End Of Interrupt signal to Local APIC
static inline void eoi() {
    write_reg(LAPIC_EOI, 0);
}

} // namespace apic