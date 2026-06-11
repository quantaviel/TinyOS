#include <arch/x86/int.h>
#include <arch/x86/iroute.h>

#include <hal/apic.h>
#include <hal/cpu.h>
#include <hal/ioapic.h>
#include <hal/keyboard.h>
#include <core/sleep.h>
#include <klib/fmt.h>
#include <ds/queue.h>

namespace keyboard {

#define SCANCODE_PORT 0x60
static CircularQueue<uint8_t, 512> kb_queue;
static sleep::waiting_queue keyboard_wq;

auto get_scancode() -> unsigned char {
    return cpu::inb(SCANCODE_PORT);
}

void keyboard_handler(int_frame_t* frame) {
    apic::eoi();
    unsigned char ch = get_scancode();
    if (!kb_queue.push(ch)) {
        fmt::println("Keyboard buffer full!");
    } else {
        sleep::wake_up_one(&keyboard_wq);
    }
}

bool read(uint8_t& scancode) {
    return kb_queue.pop(scancode);
}

auto buffer_empty() -> bool {
    return kb_queue.empty();
}

auto get_waiting_queue() -> sleep::waiting_queue* {
    return &keyboard_wq;
}

void init() {
    list_init(&keyboard_wq);
    iroute_register(IRQ_VECTOR_KEYBORAD, keyboard_handler);
}

}