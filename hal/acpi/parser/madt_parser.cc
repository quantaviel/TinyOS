#include "madt_parser.h"
#include <mm/kalloc.h>
#include <klib/assert.h>

int
madt_validate(acpi_madt_t *madt) {
    uint8_t sum = 0;
    uint32_t len = madt->header.length;
    for (int i = 0; i < len; ++i) {
        sum += *(uint8_t *)((uint8_t *)madt + i);
    }
    return sum == 0;
}

int
madt_parser(acpi_madt_t *madt, acpi_ctx_t *ctx) {
    ctx->madt = (madt_ctx_t *)mm::kvm::kalloc(sizeof(madt_ctx_t));
    ctx->madt->apic_addr = (uint32_t *)madt->apic_addr;
    // fmt::println("madt: {x}, apic_addr: {x}, len: {}", madt, (uint32_t*)madt->apic_addr, (uint32_t)madt->header.length);
    // LOG(LOG_INFO, "madt: 0x%x, apic_addr: 0x%x, len: %d", madt, madt->apic_addr, madt->header.length);
    
    assert(madt_validate(madt));


    uint8_t *ics_start = (uint8_t *)((uint8_t *)madt + sizeof(acpi_madt_t));
    // LOG(LOG_INFO, "start: 0x%x", ics_start);
    uint8_t *ics_end   = (uint8_t *)((uint8_t *)madt + madt->header.length);
    // LOG(LOG_INFO, "start: 0x%x, end: 0x%x", ics_start, ics_end);

    ctx->madt->irq_execption = (madt_iso_t **)mm::kvm::kalloc(24 * sizeof(madt_iso_t *));
    for (int i = 0; i < 24; ++i) {
        ctx->madt->irq_execption[i] = 0;
    }
    // fmt::println("irq_execption: {x}", ctx->madt->irq_execption);

    while (ics_start < ics_end) {
        uint8_t type   = *(uint8_t *)ics_start;
        uint8_t length = *(uint8_t *)(ics_start + 1);
        switch (type) {
            case 0: // local APIC
                ctx->madt->apic = (madt_apic_t *)ics_start;
                break;
            case 1: // IO APIC
                ctx->madt->ioapic = (madt_ioapic_t *)ics_start;
                break;
            case 2: { // iso
                madt_iso_t *iso = (madt_iso_t *)ics_start;
                // fmt::println("iso->irq: {}, iso: {x}", iso->irq, iso);
                ctx->madt->irq_execption[iso->irq] = iso;
                break;
            }
            default:
                break;
        }
        ics_start += length;
    }
    return 0;
}