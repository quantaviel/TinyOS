#pragma once
#ifndef FMT_H
#define FMT_H

#include <stddef.h>
#include <stdint-gcc.h>
#include <hal/vga.h>

namespace fmt {

    namespace conv {

    int to_string(int num, char *buffer, int base);
    int to_string(unsigned num, char *buffer, int base);
    int to_string(long long num, char *buffer, int base);
    int to_string(unsigned long long num, char *buffer, int base);

    int to_string(long num, char *buffer, int base);
    int to_string(unsigned long num, char *buffer, int base);
    
    } // namespace conv

template<typename T>
void print_value(T value) {
    char buf[16];
    int length = 0;

    length = conv::to_string(value, buf, 10);

    while (length--)
        vga::putchar(buf[length]);
}

template<typename T>
void print_value(T* value) { print_value((uintptr_t)value); }

void print_value(char c);
void print_value(const char* s);


template<typename T>
void print_hex(T value) {
    constexpr char hex[] = "0123456789ABCDEF";
    vga::write("0x");

    for (int i = 7; i >= 0; --i) {
        vga::putchar(hex[(value >> (i * 4)) & 0xF]);
    }
}

template<typename T>
void print_hex(T* value) { print_hex((uintptr_t)value); }

void print(const char* fmt);

template<typename T, typename... Rest>
void print(const char* fmt, T value, Rest... rest) {
    while (*fmt) {
        if (fmt[0] == '{' && fmt[1] == '}') {
            print_value(value);
            print(fmt + 2, rest...);
            return;
        }

        if (fmt[0] == '{' && fmt[1] == 'x' && fmt[2] == '}') {
            print_hex(value);
            print(fmt + 3, rest...);
            return;
        }

        vga::putchar(*fmt++);
    }
}

template<typename... Args>
void println(const char* fmt, Args&&... args) {
    print(fmt, args...);
    vga::putchar('\n');
}

void println(const char* fmt);

template<typename T>
int sprint_value(char* buffer, T value) {
    char buf[16];
    int length = 0;

    length = conv::to_string(value, buf, 10);

    for (int i = length; i--; ) {
        *buffer++ = buf[i];
    }

    *buffer = '\0';
    
    return length;
}

template<typename T>
int sprint_hex(char* buffer, T value) {
    constexpr char hex[] = "0123456789ABCDEF";
    *buffer++ = '0';
    *buffer++ = 'x';
    int u = 8;
    if (value > 0xFFFFFFFF) u = 16;

    for (int i = u - 1; i >= 0; --i) {
        *buffer++ = hex[(value >> (i * 4)) & 0xF];
    }

    *buffer = '\0';

    return 10; // length of string
}



void format(char* buffer, const char* fmt);

template<typename T, typename...Rest>
void format(char* buffer, const char* fmt, T&& value, Rest... rest) {
    while (*fmt) {
        if (fmt[0] == '{' && fmt[1] == '}') {
            int len = sprint_value(buffer, value);
            format(buffer + len, fmt + 2, rest...);
            return;
        }

        if (fmt[0] == '{' && fmt[1] == 'x' && fmt[2] == '}') {
            int len = sprint_hex(buffer, value);
            format(buffer + len, fmt + 3, rest...);
            return;
        }

        *buffer++ = *fmt++;
    }
    *buffer = '\0';
}

} // namespace fmt

#endif // FMT_H