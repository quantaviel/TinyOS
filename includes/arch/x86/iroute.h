#pragma once

#include <arch/x86/int.h>
#include <stdint-gcc.h>

// real interrupt processor
typedef void (*int_proc)(int_frame*);

void iroute_reg(uint32_t vector, int_proc handler);
void iroute_init();