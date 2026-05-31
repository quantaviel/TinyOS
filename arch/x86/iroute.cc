#include <arch/x86/int.h>
#include <arch/x86/iroute.h>
#include <hal/apic.h>
#include <klib/fmt.h>

void
__print_panic_msg(const char *msg, const int_frame *frame) {
    fmt::print("  INT {}: ({x}) [{x}: {x}] {}",
        frame->int_vec,
        frame->error_code,
        frame->cs,
        frame->eip,
        msg
    );
}


void inter_divide_error(int_frame *frame) {
    __print_panic_msg("Divide By Zero!", frame);
    while(1);
}

void inter_debug_exception(int_frame *frame) {
    __print_panic_msg("Debug Exception!", frame);
    while(1);
}

void inter_nmi(int_frame *frame) {
    __print_panic_msg("Non-Maskable Interrupt!", frame);
    while(1);
}

void inter_breakpoint(int_frame *frame) {
    __print_panic_msg("Breakpoint Exception!", frame);
    while(1);
}

void inter_overflow(int_frame *frame) {
    __print_panic_msg("Overflow Exception!", frame);
    while(1);
}

void inter_bound_range(int_frame *frame) {
    __print_panic_msg("BOUND Range Exceeded!", frame);
    while(1);
}

void inter_undefined_opcode(int_frame *frame) {
    __print_panic_msg("Invalid Opcode!", frame);
    while(1);
}

void inter_no_math_coprocessor(int_frame *frame) {
    __print_panic_msg("Device Not Available (No Math Coprocessor)!", frame);
    while(1);
}

void inter_double_fault(int_frame *frame) {
    __print_panic_msg("Double Fault!", frame);
    while(1);
}

void inter_coprocessor_overrun(int_frame *frame) {
    __print_panic_msg("Coprocessor Segment Overrun!", frame);
    while(1);
}

void inter_invalid_tss(int_frame *frame) {
    __print_panic_msg("Invalid TSS!", frame);
    while(1);
}

void inter_segment_not_present(int_frame *frame) {
    __print_panic_msg("Segment Not Present!", frame);
    while(1);
}

void inter_stack_seg_fault(int_frame *frame) {
    __print_panic_msg("Stack Segment Fault!", frame);
    while(1);
}

void inter_general_protection(int_frame *frame) {
    __print_panic_msg("General Protection Fault!", frame);
    while(1);
}

void inter_page_fault(int_frame *frame) {
    uint32_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    char buf[64];
    fmt::format(buf, "Paga Fault on {x}", cr2);
    __print_panic_msg(buf, frame);
    while(1);
}

void inter_reserved_15(int_frame *frame) {
    __print_panic_msg("Reserved Exception 15!", frame);
    while(1);
}

void inter_math_fault(int_frame *frame) {
    __print_panic_msg("x87 Floating-Point Exception!", frame);
    while(1);
}

void inter_alignment_check(int_frame *frame) {
    __print_panic_msg("Alignment Check!", frame);
    while(1);
}

void inter_machine_check(int_frame *frame) {
    __print_panic_msg("Machine Check!", frame);
    while(1);
}

void inter_simd_fp_exception(int_frame *frame) {
    __print_panic_msg("SIMD Floating-Point Exception!", frame);
    while(1);
}

void inter_virtualization(int_frame *frame) {
    __print_panic_msg("Virtualization Exception!", frame);
    while(1);
}

void inter_control_protection(int_frame *frame) {
    __print_panic_msg("Control Protection Exception!", frame);
    while(1);
}

static int_proc proc[256];

extern "C"
void int_handler(int_frame* frame) {
    if (!proc[frame->int_vec]) {
        fmt::println("Unkown interrupt!");
    } else {
        proc[frame->int_vec](frame);
    }
    if (frame->int_vec >= EX_INTER_BEGIN && frame->int_vec != IRQ_VECTOR_LAPIC_SPURIOUS) {
        apic::eoi();
    }
}


void iroute_init() {
    iroute_reg(IRQ_VECTOR_DIVIDE_ERROR, inter_divide_error);
    iroute_reg(IRQ_VECTOR_DEBUG_EXCEPTION, inter_debug_exception);
    iroute_reg(2, inter_nmi);  // NMI
    iroute_reg(IRQ_VECTOR_BREAKPOINT, inter_breakpoint);
    iroute_reg(IRQ_VECTOR_OVERFLOW, inter_overflow);
    iroute_reg(IRQ_VECTOR_BOUND_RANGE, inter_bound_range);
    iroute_reg(IRQ_VECTOR_UNDEFINED_OPCODE, inter_undefined_opcode);
    iroute_reg(IRQ_VECTOR_NO_MATH_COPROCESSOR, inter_no_math_coprocessor);
    iroute_reg(IRQ_VECTOR_DOUBLE_FAULT, inter_double_fault);
    iroute_reg(9, inter_coprocessor_overrun);  // obsolete
    iroute_reg(IRQ_VECTOR_INVALID_TSS, inter_invalid_tss);
    iroute_reg(IRQ_VECTOR_SEGMENT_NOT_PRESENT, inter_segment_not_present);
    iroute_reg(IRQ_VECTOR_STACK_SEG_FAULT, inter_stack_seg_fault);
    iroute_reg(IRQ_VECTOR_GENERAL_PROTECTION, inter_general_protection);
    iroute_reg(IRQ_VECTOR_PAGE_FAULT, inter_page_fault);
    iroute_reg(15, inter_reserved_15);  // Reserved
    iroute_reg(IRQ_VECTOR_MATH_FAULT, inter_math_fault);
    iroute_reg(IRQ_VECTOR_ALIGNMENT_CHECK, inter_alignment_check);
    iroute_reg(IRQ_VECTOR_MACHINE_CHECK, inter_machine_check);
    iroute_reg(IRQ_VECTOR_SIMD_FP_EXCEPTION, inter_simd_fp_exception);
    iroute_reg(IRQ_VECTOR_VIRTUALIZATION, inter_virtualization);
    iroute_reg(IRQ_VECTOR_CONTROL_PROTECTION, inter_control_protection);
}

void iroute_reg(uint32_t vector, int_proc handler) {
    proc[vector] = handler;
}