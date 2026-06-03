#include <arch/x86/int.h>
#include <arch/x86/iroute.h>

#include <syscall.h>
#include <process.h>

#include <hal/keyboard.h>

#include <klib/fmt.h>

#include <stdint-gcc.h>
#include <stddef.h>

#define _syscall0(type, name) \
type name(void) { \
    long __res; \
    __asm__ __volatile__ ( \
        "int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##name) \
        : "memory" \
    ); \
    if (__res >= 0) \
        return (type) __res; \
    return (type) -1; \
}

#define _syscall1(type, name, type1, arg1) \
type name(type1 arg1) { \
    long __res; \
    __asm__ __volatile__ ( \
        "int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##name), "b" ((long)(arg1)) \
        : "memory" \
    ); \
    if (__res >= 0) \
        return (type) __res; \
    return (type) -1; \
}

#define _syscall3(type, name, type1, arg1, type2, arg2, type3, arg3) \
type name(type1 arg1, type2 arg2, type3 arg3) { \
    long __res; \
    __asm__ __volatile__ ( \
        "int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##name), "b" ((long)(arg1)), "c" ((long)(arg2)), "d" ((long)(arg3)) \
        : "memory" \
    ); \
    if (__res >= 0) \
        return (type) __res; \
    return (type) -1; \
}

int32_t read(int fd, void* buf, size_t size) {
    long __res;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a" (__res)
        : "0" (__NR_read), "b" ((long)(fd)), "c" ((long)(buf)), "d" ((long)(size))
        : "memory"
    );
    if (__res >= 0)
        return (int32_t) __res;
    return (int32_t) -1;
}

int32_t write(int fd, const void* buf, size_t size) {
    long __res;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a" (__res)
        : "0" (__NR_write), "b" ((long)(fd)), "c" ((long)(buf)), "d" ((long)(size))
        : "memory"
    );
    if (__res >= 0)
        return (int32_t) __res;
    return (int32_t) -1;
}

proc::pid_t fork() {
    long __res;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a" (__res)
        : "0" (__NR_fork)
        : "memory"
    );
    if (__res >= 0)
        return (proc::pid_t) __res;
    return (proc::pid_t) -1;
}

int32_t sys_read(int fd, void* buf, size_t count) {
    if (fd != 0) {
        return -1;  // Only stdin (fd=0) is supported
    }

    if (buf == nullptr || count == 0) {
        return -1;
    }

    char* char_buf = static_cast<char*>(buf);
    size_t bytes_read = 0;

    while (bytes_read < count) {
        uint8_t scancode;
        if (!keyboard::read(scancode)) {
            break;  // No more data in buffer
        }
        char_buf[bytes_read++] = static_cast<char>(scancode);
    }

    return bytes_read == 0 ? -1 : static_cast<int32_t>(bytes_read);
}

int32_t sys_write(int fd, const void* buf, size_t size) {
    for (int i = 0; i < size; ++i) {
        vga::putchar(((const char*)buf)[i]);
    }
    return size;
}

void syscall_handler(int_frame_t* frame) {
    uint32_t call_numer = frame->gp.eax;
    uint32_t arg1 = frame->gp.ebx;
    uint32_t arg2 = frame->gp.ecx;
    uint32_t arg3 = frame->gp.edx;
    switch (call_numer) {
        case __NR_read:
            sys_read(arg1, (void *)arg2, arg3);
            break;
        case __NR_write:
            sys_write(arg1, (const void *)arg2, arg3);
            break;
        case __NR_fork:
            proc::sys_fork(frame);
            break;
        default:
            break;
    }
}

void syscall_init() {
    iroute_register(IRQ_VECTOR_SYSTEM_CALLS, syscall_handler);
}
