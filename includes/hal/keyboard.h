#pragma once

#define KEYBOARD_APIC_INT_VEC 205
#define KEYBOARD_INT_IRQ      1

namespace keyboard {

void init();
bool read(uint8_t& scancode);

} // namespace keyboard