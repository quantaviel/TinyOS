#include <stddef.h>

#include <hal/acpi/acpi.h>
#include <mm/kalloc.h>
#include <mm/memory.h>
#include <mm/vmm.h>
#include <klib/fmt.h>
#include <klib/assert.h>

#include "./parser/madt_parser.h"

acpi_rsdp_t*
acpi_find_rsdp() {
    acpi_rsdp_t *rsdp = NULL;
    for (uint32_t addr = 0x4000; addr < 0x100000; addr += 0x10) {
        uint32_t sign_l = *(uint32_t *)addr;
        if (sign_l == ACPI_RSD_SIG_L) {
            uint32_t sign_h = *(((uint32_t *)addr) + 1);
            if (sign_h == ACPI_RSD_SIG_H) {
                rsdp = (acpi_rsdp_t *)addr;
                break;
            }
        }
    }
    return rsdp;
}

int acpi_init();

acpi_ctx_t* ctx;

acpi_ctx_t*
acpi_get_context() {
    if (ctx == NULL) {
        if (acpi_init()) return NULL;
    }
    return ctx;
}

int
acpi_rsdp_validate(acpi_rsdp_t *rsdp) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < 20; ++i) {
        sum += *(uint8_t*)((uint8_t *)rsdp + i);
    }
    return sum == 0;
}

int
acpi_rsdt_validate(acpi_rsdt_t *rsdt) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < rsdt->header.length; ++i) {
        sum += *(uint8_t*)((uint8_t *)rsdt + i);
    }
    return sum == 0;
}

int
acpi_init() {
    ctx = (acpi_ctx_t *)mm::kvm::kalloc(sizeof(acpi_ctx_t));

    acpi_rsdp_t *rsdp = acpi_find_rsdp();
    acpi_rsdt_t *rsdt = (acpi_rsdt_t *)rsdp->rsdt_addr;

    ctx->rsdp = rsdp;
    ctx->rsdt = rsdt;

    assert(acpi_rsdp_validate(ctx->rsdp));
    assert(acpi_rsdt_validate(ctx->rsdt));

    uint8_t *start = ((uint8_t *)ctx->rsdt) + sizeof(acpi_sdt_header_t);
    uint8_t *end   = ((uint8_t *)ctx->rsdt) + ctx->rsdt->header.length;
    uint32_t entry_count = (end - start) / 4;
    for (int i = 0; i < entry_count; ++i) {
        acpi_sdt_header_t *header = (acpi_sdt_header_t *) ((uint32_t *)rsdt->entry)[i];
        switch(header->signature) {
            case ACPI_MADT_SIG:
                madt_parser((acpi_madt_t *)header, ctx);
                break;
            default:
                break;
        }
    }

    
    uint32_t *apic_addr = ctx->madt->apic_addr;
    uint32_t *ioapic_addr = (uint32_t *)ctx->madt->ioapic->addr;
    uint32_t *oem_id = (uint32_t *)ctx->rsdp->oem_id;

    fmt::println("ACPI Addr: {x}, IOAPIC Addr: {x}", apic_addr, ioapic_addr);
    // LOG(LOG_INFO, "APIC Addr: 0x%x, IOAPIC Addr: 0x%x", apic_addr, ioapic_addr);

    for (int i = 0; i < 24; ++i) {
        if (!ctx->madt->irq_execption[i]) continue;
        // fmt::println("ctx->madt->irq_execption: {x}", (uint32_t)ctx->madt->irq_execption+i);
        madt_iso_t *iso = ctx->madt->irq_execption[i];
        // fmt::println("iso: {x}", iso);
        // fmt::println("iso->type: {}", iso->type);
        // fmt::println("iso->gsi: {x}", (uint32_t)iso->gsi);
        fmt::println("Mapping irq {} -> {}", iso->irq, (uint32_t)iso->gsi);
        // LOG(LOG_INFO, "Mapping irq %d -> gsi %d", iso->irq, iso->gsi);
    }
    
    return 0;
}