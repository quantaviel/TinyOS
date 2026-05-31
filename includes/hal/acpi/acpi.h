#ifndef HAL_ACPI_ACPI_H
#define HAL_ACPI_ACPI_H

#include <stdint-gcc.h>

#include <hal/acpi/madt.h>
#include <hal/acpi/rsdp.h>
#include <hal/acpi/sdt.h>

#define ACPI_RSD_SIG_L 0x20445352 // "RSD "
#define ACPI_RSD_SIG_H 0x20525450 // "PTR "

/*
 * RSDP (Root System Description Pointer)
 */

/* madt_ctx_t and acpi_ctx_t follow */

typedef struct {
    uint32_t      *apic_addr;          // Local APIC address
    madt_apic_t   *apic;
    madt_ioapic_t *ioapic;
    madt_iso_t   **irq_execption;   // Identity mapping for irq
} madt_ctx_t;

typedef struct {
    acpi_rsdp_t *rsdp;
    acpi_rsdt_t *rsdt;
    madt_ctx_t  *madt;
} acpi_ctx_t;

acpi_ctx_t* acpi_get_context();


#endif