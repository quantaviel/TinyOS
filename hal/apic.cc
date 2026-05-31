#include <arch/x86/iroute.h>
#include <hal/apic.h>
#include <hal/cpu.h>
#include <klib/fmt.h>
#include <klib/assert.h>
#include <mm/vmm.h>

namespace apic {

void init() {
    cpu::disable_interrupts();

    // APIC enable
    enable();

    uintptr_t apic_base = get_base_addr();
    uint32_t apic_id = get_id();
    uint32_t apic_version = get_version();
    fmt::println("Local APIC initialized at {x}, ID: {}, Version: {}", apic_base, apic_id, apic_version);

    // set TPR
    write_reg(LAPIC_TPR, LAPIC_TPR_CLASS(2) | LAPIC_TPR_SUBCL(0));

    // init LVT LINT0 LINT1
    write_reg(LAPIC_LVT_LINT0, LAPIC_LVT_FIXED | LAPIC_LVT_LINT0_VEC);
    write_reg(LAPIC_LVT_LINT1, LAPIC_LVT_NMI | LAPIC_LVT_MASK | LAPIC_LVT_TRIG_EDGE);
    write_reg(LAPIC_LVT_ERROR, LAPIC_LVT_FIXED | LAPIC_LVT_ERROR_VEC);

    // set SVR
    uint32_t svr = read_reg(LAPIC_SVR);
    svr = (svr & ~(0xFF)) | LAPIC_SVR_LAPIC_ENABLE | LAPIC_SVR_VEC;
    write_reg(LAPIC_SVR, svr);

    cpu::enable_interrupts();

    // Timer test removed - now handled by timer::init()
    static int test_handled = 0;
    iroute_reg(LAPIC_LVT_TIMER_VEC, [](int_frame* frame) {
        fmt::println("LAPIC Timer Interrupt received.");
        test_handled = 1;
        apic::eoi();
    });
    
    write_reg(LAPIC_LVT_TIMER, LAPIC_TIMER_ONE_SHOT | LAPIC_LVT_TIMER_VEC);
    write_reg(LAPIC_DIVIDE_CONF, LAPIC_TIMER_DIVIDE_64);
    write_reg(LAPIC_INIT_COUNT, 0x1000);
    
    while (!test_handled);
}

void enable() {
    uint32_t low, high;
    cpu::read_msr(IA32_APIC_BASE_MSR, &low, &high);
    low |= IA32_APIC_BASE_MSR_ENABLE;
    cpu::write_msr(IA32_APIC_BASE_MSR, low, high);
}

uintptr_t get_base_addr() {
    uint32_t low, high;
    cpu::read_msr(IA32_APIC_BASE_MSR, &low, &high);
    return (uintptr_t)(low & 0xFFFFF000);
}

void set_base_addr(uintptr_t addr) {
    uint32_t low, high;
    cpu::read_msr(IA32_APIC_BASE_MSR, &low, &high);
    low = (low & 0x00000FFF) | (addr & 0xFFFFF000);
    cpu::write_msr(IA32_APIC_BASE_MSR, low, high);
}

void set_apic_timer(uint32_t option, void (func)(int_frame*), uint32_t divide, uint32_t count) {
    cpu::disable_interrupts();
    
    iroute_reg(LAPIC_LVT_TIMER_VEC, func);
    write_reg(LAPIC_LVT_TIMER, option | LAPIC_LVT_TIMER_VEC);
    write_reg(LAPIC_DIVIDE_CONF, divide);
    write_reg(LAPIC_INIT_COUNT, count);
}

} // namespace apic