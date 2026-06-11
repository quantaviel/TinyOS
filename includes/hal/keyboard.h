#pragma once

#include <core/sleep.h>

#define KEYBOARD_APIC_INT_VEC 205
#define KEYBOARD_INT_IRQ      1

namespace keyboard {

void init();
auto read(uint8_t& scancode) -> bool;

auto buffer_empty() -> bool;
auto get_waiting_queue() -> sleep::waiting_queue*;

} // namespace keyboard