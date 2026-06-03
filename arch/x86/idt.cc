#include <arch/x86/idt.h>
#include <arch/x86/int.h>
#include <arch/x86/iroute.h>

#include <klib/fmt.h>

uint64_t idt[256];

void idt_set_entry(int index, uint32_t offset, uint32_t selector, uint32_t type, uint32_t access) {
    idt[index] = 0;
    idt[index] = (offset & 0xFFFF0000) | IDT_PRESENT |
                 type | IDT_SIZE32 | access;
    idt[index] <<= 32;
    idt[index] |= (selector << 16) | (offset & 0x0000FFFF);
}

void idt_load() {
    idtr_t idt_ptr;
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)idt;

    asm volatile(
        "lidt %0" : : "m"(idt_ptr)
    );
}

extern "C"
void idt_init() {
    iroute_init();
    idt_set_entry(IRQ_VECTOR_DIVIDE_ERROR, (uint32_t)_asm_isr_0, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_DEBUG_EXCEPTION, (uint32_t)_asm_isr_1, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(2, (uint32_t)_asm_isr_2, 0x08, IDT_INT_GATE, IDT_RING0); // NMI
    idt_set_entry(IRQ_VECTOR_BREAKPOINT, (uint32_t)_asm_isr_3, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_OVERFLOW, (uint32_t)_asm_isr_4, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_BOUND_RANGE, (uint32_t)_asm_isr_5, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_UNDEFINED_OPCODE, (uint32_t)_asm_isr_6, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_NO_MATH_COPROCESSOR, (uint32_t)_asm_isr_7, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_DOUBLE_FAULT, (uint32_t)_asm_isr_8, 0x08, IDT_INT_GATE, IDT_RING0);
    // Coprocessor segment overrun (obsolete)
    idt_set_entry(9, (uint32_t)_asm_isr_9, 0x08, IDT_INT_GATE, IDT_RING0); 
    idt_set_entry(IRQ_VECTOR_INVALID_TSS, (uint32_t)_asm_isr_10, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_SEGMENT_NOT_PRESENT, (uint32_t)_asm_isr_11, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_STACK_SEG_FAULT, (uint32_t)_asm_isr_12, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_GENERAL_PROTECTION, (uint32_t)_asm_isr_13, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_PAGE_FAULT, (uint32_t)_asm_isr_14, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(15, (uint32_t)_asm_isr_15, 0x08, IDT_INT_GATE, IDT_RING0); // Reserved
    idt_set_entry(IRQ_VECTOR_MATH_FAULT, (uint32_t)_asm_isr_16, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_ALIGNMENT_CHECK, (uint32_t)_asm_isr_17, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_MACHINE_CHECK, (uint32_t)_asm_isr_18, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_SIMD_FP_EXCEPTION, (uint32_t)_asm_isr_19, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_VIRTUALIZATION, (uint32_t)_asm_isr_20, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_CONTROL_PROTECTION, (uint32_t)_asm_isr_21, 0x08, IDT_INT_GATE, IDT_RING0);

    idt_set_entry(IRQ_VECTOR_SYSTEM_CALLS, (uint32_t)_asm_isr_128, 0x08, IDT_TRAP_GATE, IDT_RING3);

    idt_set_entry(IRQ_VECTOR_LAPIC_ERROR, (uint32_t)_asm_isr_200, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_LAPIC_LINT0, (uint32_t)_asm_isr_201, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_LAPIC_SPURIOUS, (uint32_t)_asm_isr_202, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_LAPIC_TIMER, (uint32_t)_asm_isr_203, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_RTC_TIMER, (uint32_t)_asm_isr_204, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_set_entry(IRQ_VECTOR_KEYBORAD, (uint32_t)_asm_isr_205, 0x08, IDT_INT_GATE, IDT_RING0);
    idt_load();
    // fmt::println("idt_init finish!");
}