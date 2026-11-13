#include <hal/acpi/acpi.h>
#include <hal/ioapic.h>
#include <hal/keyboard.h>
#include <hal/rtc.h>
#include <klib/fmt.h>

namespace ioapic {

uint32_t read(uint32_t offset) {
    IOAPIC_IOREGSEL = offset;
    return IOAPIC_IOWIN;
}

void write(uint32_t offset, uint32_t value) {
    IOAPIC_IOREGSEL = offset;
    IOAPIC_IOWIN = value;
}

int get_version();
uint32_t get_irq(acpi_ctx_t *ctx, uint8_t old_irq);
void redirect(  uint8_t irq,
                uint8_t intvec,
                uint32_t masked,
                uint32_t trigger,
                uint32_t polarity,
                uint32_t destmod,
                uint32_t delmod,
                uint32_t destaddr );
void init() {
    acpi_ctx_t *ctx = acpi_get_context();
    fmt::println("IOAPIC Version: {}", get_version());

    // redirect rtc irq (8) to rtc timer vector (204)
    redirect(
        get_irq(ctx, RTC_INT_IRQ),
        RTC_TIMER_IRQ,
        0,
        IOAPIC_TRIGMOD_EDGE,
        IOAPIC_INTPOL_HIGH,
        IOAPIC_DESTMOD_PHY,
        IOAPIC_DELMOD_FIXED,
        0 // APIC ID = 0
    );

    // redirect keyboard irq (1) to keyboard vector (205)
    redirect(
        get_irq(ctx, KEYBOARD_INT_IRQ),
        KEYBOARD_APIC_INT_VEC,
        0,
        IOAPIC_TRIGMOD_EDGE,
        IOAPIC_INTPOL_HIGH,
        IOAPIC_DESTMOD_PHY,
        IOAPIC_DELMOD_FIXED,
        0
    );
}

int get_version() {
    uint32_t x = read(IOAPIC_VER);
    return x & 0xFF;
}
uint32_t get_irq(acpi_ctx_t *ctx, uint8_t old_irq) {
    madt_iso_t **remapped_irq = ctx->madt->irq_execption;
    if (remapped_irq[old_irq] != NULL) return remapped_irq[old_irq]->gsi;
    return old_irq;
}
void redirect(
    uint8_t irq,
    uint8_t intvec,
    uint32_t masked,
    uint32_t trigger,
    uint32_t polarity,
    uint32_t destmod,
    uint32_t delmod,
    uint32_t destaddr
) {
    uint32_t lowbit = intvec | IOAPIC_SET_DELMOD(delmod) | IOAPIC_SET_DESTMOD(destmod)
        | IOAPIC_SET_INTPOL(polarity) | IOAPIC_SET_TRIGMOD(trigger);
    if (masked) lowbit |= IOAPIC_SET_MASK;

    uint32_t highbit = IOAPIC_SET_DESTADDR(destaddr);

    write(IOAPIC_IOREDTBL_LOW(irq), lowbit);
    write(IOAPIC_IOREDTBL_HIG(irq), highbit);
}

} // namespace ioapic