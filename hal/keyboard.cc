#include <arch/x86/int.h>
#include <arch/x86/iroute.h>

#include <hal/apic.h>
#include <hal/cpu.h>
#include <hal/ioapic.h>
#include <hal/keyboard.h>

#include <klib/fmt.h>
#include <queue.h>

namespace keyboard {

#define SCANCODE_PORT 0x60
static CircularQueue<uint8_t, 512> kb_queue;

auto get_scancode() -> unsigned char {
    return cpu::inb(SCANCODE_PORT);
}

void keyboard_handler(int_frame_t* frame) {
    apic::eoi();
    unsigned char ch = get_scancode();
    if (!kb_queue.push(ch)) {
        fmt::println("Keyboard buffer full!");
    }
}

bool read(uint8_t& scancode) {
    return kb_queue.pop(scancode);
}

void init() {
    iroute_register(IRQ_VECTOR_KEYBORAD, keyboard_handler);
}

}