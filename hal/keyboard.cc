#include <arch/x86/int.h>
#include <arch/x86/iroute.h>

#include <hal/apic.h>
#include <hal/cpu.h>
#include <hal/ioapic.h>
#include <hal/keyboard.h>
#include <core/sleep.h>
#include <core/process.h>
#include <klib/fmt.h>
#include <ds/queue.h>

namespace keyboard {

#define SCANCODE_PORT 0x60
static circular_queue<uint8_t, 512> kb_queue(true);
static sleep::waiting_queue keyboard_wq;

// PS/2 Set 1 unshifted make codes. Zero means no character.
static constexpr char set1_chars[128] = {
    // 0x00
    0,   0,   '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', '\b', '\t',
    // 0x10
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', 0,  'a', 's',
    // 0x20
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0,  '\\', 'z', 'x', 'c', 'v',
    // 0x30
    'b', 'n', 'm', ',', '.', '/', 0,   0,
    0,   ' ',
};

uint8_t get_scancode() {
    return cpu::inb(SCANCODE_PORT);
}

void keyboard_handler(int_frame_t* frame) {
    apic::eoi();
    uint8_t ch = get_scancode();
    kb_queue.push(ch); // allow overflow
    sleep::wake_up_one(&keyboard_wq);
    proc::schedule();
}

bool read(uint8_t& scancode) {
    return kb_queue.pop(scancode);
}

bool buffer_empty() {
    return kb_queue.empty();
}

sleep::waiting_queue* get_waiting_queue() {
    return &keyboard_wq;
}

char map(uint8_t scancode) {
    if (scancode > 0x7f) return 0;
    return set1_chars[scancode];
}

void init() {
    list_init(&keyboard_wq);
    iroute_register(IRQ_VECTOR_KEYBORAD, keyboard_handler);
}

}
