#pragma once

#include <stdint-gcc.h>

namespace vga {

enum vga_color {
	COLOR_BLACK = 0,
	COLOR_BLUE = 1,
	COLOR_GREEN = 2,
	COLOR_CYAN = 3,
	COLOR_RED = 4,
	COLOR_MAGENTA = 5,
	COLOR_BROWN = 6,
	COLOR_LIGHT_GREY = 7,
	COLOR_DARK_GREY = 8,
	COLOR_LIGHT_BLUE = 9,
	COLOR_LIGHT_GREEN = 10,
	COLOR_LIGHT_CYAN = 11,
	COLOR_LIGHT_RED = 12,
	COLOR_LIGHT_MAGENTA = 13,
	COLOR_LIGHT_BROWN = 14,
	COLOR_WHITE = 15,
};

constexpr static int VGA_WIDTH = 80;
constexpr static int VGA_HEIGHT = 25;
constexpr static int VGA_PHY_ADDR = 0xB8000;
constexpr static int VGA_VIR_ADDR = 0xB0000000;
constexpr static int VGA_BUFFER_SIZE = 0x1000;

struct writer {
    struct screen_char {
        uint8_t ascii;
        uint8_t attr;
    } __attribute__((packed));
    uint8_t color_attr;
    int current_pos;
    screen_char* buffer;
};

#define MAKE_COLOR_ATTR(front, back) (((back) << 4) | (front))

void putchar(char ch);
void write(const char* str);
char recieve(const char&);

void set_color(vga_color front, vga_color back);
void set_addr(uintptr_t addr);
void set_pos(int pos);

} // namespace vga;