#include <arch/x86/int.h>
#include <arch/x86/iroute.h>

#include <hal/apic.h>
#include <hal/cpu.h>
#include <hal/ioapic.h>
#include <hal/keyboard.h>

#include <klib/fmt.h>

namespace keyboard {

#define SCANCODE_PORT 0x60
#define BUFFER_SIZE 512
static uint8_t kb_buf[512];
static int head, tail, curr_buf_size;

auto get_scancode() -> unsigned char {
    return cpu::inb(SCANCODE_PORT);
}

void keyboard_handler(int_frame* frame) {
    apic::eoi();
    unsigned char ch = get_scancode();
    if (curr_buf_size == BUFFER_SIZE) // Buffer is full, dropout it.
        return;
    tail = (tail + 1) % BUFFER_SIZE;
    curr_buf_size += 1; // !!! maybe concurency problem !!!
    kb_buf[tail] = ch;
}

void init() {
    head = 0, tail = BUFFER_SIZE - 1; curr_buf_size = 0;
    iroute_reg(IRQ_VECTOR_KEYBORAD, keyboard_handler);
}

}