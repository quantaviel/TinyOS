#ifndef HAL_ACPI_MADT_H
#define HAL_ACPI_MADT_H


#include <hal/acpi/sdt.h>

#define ACPI_MADT_SIG   0x43495041  // "APIC"

typedef struct {
    acpi_sdt_header_t header;
    uint32_t apic_addr;
    uint32_t flags;
} __attribute__((packed)) acpi_madt_t;

typedef struct {
    uint8_t  type;              // 0 = Processor Local APIC
    uint8_t  length;            // 8 bytes
    uint8_t  cpu_uid;           // ACPI Processor UID
    uint8_t  apic_id;           // Local APIC ID
    uint32_t flags;
    /*
        bit 0: enabled
        bit 1: online-capable
    */
} __attribute__((packed)) madt_apic_t;

typedef struct {
    uint8_t  type;              // 1 = IO APIC
    uint8_t  length;            // 12 bytes
    uint8_t  id;                // IO APIC ID
    uint8_t  reserved;
    uint32_t addr;              // MMIO base address
    uint32_t gsi_base;          // Global System Interrupt base
} __attribute__((packed)) madt_ioapic_t;

typedef struct {
    uint8_t  type;              // 2 = Interrupt Source Override
    uint8_t  length;            // 10 bytes
    uint8_t  bus;               // 0 = ISA
    uint8_t  irq;               // ISA IRQ number
    uint32_t gsi;               // Mapped Global System Interrupt
    uint16_t flags;             // Polarity & Trigger mode
} __attribute__((packed)) madt_iso_t;


#endif