#include <hal/vga.h>

#include <hal/cpu.h>

namespace vga {

static writer WRITER{
    /*color_attr=*/MAKE_COLOR_ATTR(COLOR_LIGHT_GREY, COLOR_BLACK),
    /*current_pos=*/0,
    /*buffer=*/(writer::screen_char *)0xB8000
};

void set_cursor(int);
void scroll_up();

void newline();
void gofront();

void putchar(char ch) {
    switch (ch) {
        case '\n':
            newline();
            break;
        case '\r':
            gofront();
            break;
        case '\t':
            for (int _ = 0; _ < 4; ++_)
                putchar(' ');
            break;
        default:
            if (WRITER.current_pos >= VGA_WIDTH * VGA_HEIGHT) {
                scroll_up();
            }
            WRITER.buffer[WRITER.current_pos].ascii = ch;
            WRITER.buffer[WRITER.current_pos].attr = WRITER.color_attr;
            WRITER.current_pos++;
    }
    set_cursor(WRITER.current_pos);
}

void write(const char* str) {
    while (*str) putchar(*str++);
}

void set_color(vga_color front, vga_color back) {
    WRITER.color_attr = MAKE_COLOR_ATTR(front, back);
}
void set_addr(uintptr_t addr) {
    WRITER.buffer = (writer::screen_char*)addr;
}
void set_pos(int pos) {
    WRITER.current_pos = pos;
}

void newline() {
    WRITER.current_pos = (WRITER.current_pos / VGA_WIDTH) * VGA_WIDTH + VGA_WIDTH;
}
void gofront() {
    WRITER.current_pos = (WRITER.current_pos / VGA_WIDTH - 1) * VGA_WIDTH;
}

void set_cursor(int pos) {
    cpu::outb(0x3D4, 0x0F);
    cpu::outb(0x3D5, pos & 0xFF);

    cpu::outb(0x3D4, 0x0E);
    cpu::outb(0x3D5, (pos >> 8) & 0xFF);
}

void scroll_up() {
    // Move all rows up by one line
    for (int row = 0; row < VGA_HEIGHT - 1; ++row) {
        for (int col = 0; col < VGA_WIDTH; ++col) {
            WRITER.buffer[row * VGA_WIDTH + col] = WRITER.buffer[(row + 1) * VGA_WIDTH + col];
        }
    }
    // Clear the last row
    for (int col = 0; col < VGA_WIDTH; ++col) {
        WRITER.buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col].ascii = 0;
        WRITER.buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col].attr = WRITER.color_attr;
    }
    WRITER.current_pos = (VGA_HEIGHT - 1) * VGA_WIDTH;
}

} // namespace vga


