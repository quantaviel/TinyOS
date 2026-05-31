#pragma once

#define IRQ_VECTOR_DIVIDE_ERROR           0
#define IRQ_VECTOR_DEBUG_EXCEPTION        1

#define IRQ_VECTOR_BREAKPOINT             3
#define IRQ_VECTOR_OVERFLOW               4
#define IRQ_VECTOR_BOUND_RANGE            5
#define IRQ_VECTOR_UNDEFINED_OPCODE       6
#define IRQ_VECTOR_NO_MATH_COPROCESSOR    7
#define IRQ_VECTOR_DOUBLE_FAULT           8

#define IRQ_VECTOR_INVALID_TSS            10
#define IRQ_VECTOR_SEGMENT_NOT_PRESENT    11
#define IRQ_VECTOR_STACK_SEG_FAULT        12
#define IRQ_VECTOR_GENERAL_PROTECTION     13
#define IRQ_VECTOR_PAGE_FAULT             14

#define IRQ_VECTOR_MATH_FAULT             16
#define IRQ_VECTOR_ALIGNMENT_CHECK        17
#define IRQ_VECTOR_MACHINE_CHECK          18
#define IRQ_VECTOR_SIMD_FP_EXCEPTION      19
#define IRQ_VECTOR_VIRTUALIZATION         20
#define IRQ_VECTOR_CONTROL_PROTECTION     21

#define IRQ_VECTOR_LAPIC_ERROR            200
#define IRQ_VECTOR_LAPIC_LINT0            201
#define IRQ_VECTOR_LAPIC_SPURIOUS         202
#define IRQ_VECTOR_LAPIC_TIMER            203
#define IRQ_VECTOR_RTC_TIMER              204
#define IRQ_VECTOR_KEYBORAD               205

#define EX_INTER_BEGIN                    200

#ifndef __ASM_FILE__

#include <hal/cpu.h>

struct int_frame {
    cpu::gp_regs gp;

    unsigned int_vec;
    unsigned error_code;

    unsigned eip;
    unsigned cs;
    unsigned eflags;
    unsigned esp;
    unsigned ss;
}__attribute__((packed));

extern "C" {
void _asm_isr_0();
void _asm_isr_1();
void _asm_isr_2();
void _asm_isr_3();
void _asm_isr_4();
void _asm_isr_5();
void _asm_isr_6();
void _asm_isr_7();
void _asm_isr_8();
void _asm_isr_9();
void _asm_isr_10();
void _asm_isr_11();
void _asm_isr_12();
void _asm_isr_13();
void _asm_isr_14();
void _asm_isr_15();
void _asm_isr_16();
void _asm_isr_17();
void _asm_isr_18();
void _asm_isr_19();
void _asm_isr_20();
void _asm_isr_21();
void _asm_isr_200();
void _asm_isr_201();
void _asm_isr_202();
void _asm_isr_203();
void _asm_isr_204();
void _asm_isr_205();
}

#endif