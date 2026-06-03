#pragma once

#include <arch/x86/int.h>
#include <stdint-gcc.h>

// real interrupt processor
typedef void (*int_handler_t)(int_frame_t*);

void iroute_register(uint32_t vector, int_handler_t handler);
void iroute_init(void);