#define __ASM_FILE__
#include <arch/x86/int.h>

.macro isr_template vector, no_error_code=1
.global _asm_isr_\vector
.type _asm_isr_\vector, @function
_asm_isr_\vector:
    .if \no_error_code
        pushl $0x0
    .endif
    pushl $\vector
    jmp interrupt_wrapper
.endm

.section .text
isr_template IRQ_VECTOR_DIVIDE_ERROR
isr_template IRQ_VECTOR_DEBUG_EXCEPTION, no_error_code=1
isr_template 2, no_error_code=1
isr_template IRQ_VECTOR_BREAKPOINT, no_error_code=1
isr_template IRQ_VECTOR_OVERFLOW, no_error_code=1
isr_template IRQ_VECTOR_BOUND_RANGE, no_error_code=1
isr_template IRQ_VECTOR_UNDEFINED_OPCODE, no_error_code=1
isr_template IRQ_VECTOR_NO_MATH_COPROCESSOR, no_error_code=1
isr_template IRQ_VECTOR_DOUBLE_FAULT, no_error_code=1
isr_template 9, no_error_code=1
isr_template IRQ_VECTOR_INVALID_TSS, no_error_code=0
isr_template IRQ_VECTOR_SEGMENT_NOT_PRESENT, no_error_code=0
isr_template IRQ_VECTOR_STACK_SEG_FAULT, no_error_code=0
isr_template IRQ_VECTOR_GENERAL_PROTECTION, no_error_code=0
isr_template IRQ_VECTOR_PAGE_FAULT, no_error_code=0
isr_template 15, no_error_code=1
isr_template IRQ_VECTOR_MATH_FAULT, no_error_code=1
isr_template IRQ_VECTOR_ALIGNMENT_CHECK, no_error_code=0
isr_template IRQ_VECTOR_MACHINE_CHECK, no_error_code=1
isr_template IRQ_VECTOR_SIMD_FP_EXCEPTION, no_error_code=1
isr_template IRQ_VECTOR_VIRTUALIZATION, no_error_code=1
isr_template IRQ_VECTOR_CONTROL_PROTECTION, no_error_code=1

isr_template IRQ_VECTOR_SYSTEM_CALLS

isr_template IRQ_VECTOR_LAPIC_ERROR
isr_template IRQ_VECTOR_LAPIC_LINT0
isr_template IRQ_VECTOR_LAPIC_SPURIOUS
isr_template IRQ_VECTOR_LAPIC_TIMER
isr_template IRQ_VECTOR_RTC_TIMER
isr_template IRQ_VECTOR_KEYBORAD

interrupt_wrapper:

    pushl %esp
    pushl %esi
    pushl %ebp
    pushl %edi
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax

    movl %esp, %eax
    andl $0xfffffff0, %esp
    subl $16, %esp
    movl %eax, (%esp)

    call int_handler

    popl %esp

    popl %eax
    popl %ebx
    popl %ecx
    popl %edx
    popl %edi
    popl %ebp
    popl %esi
    popl %esp

    addl $8, %esp

    iret