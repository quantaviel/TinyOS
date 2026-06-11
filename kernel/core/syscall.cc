#include <arch/x86/int.h>
#include <arch/x86/iroute.h>

#include <core/syscall.h>
#include <core/process.h>

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
        : "0" (SYS_##name) \
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
        : "0" (SYS_##name), "b" ((long)(arg1)) \
        : "memory" \
    ); \
    if (__res >= 0) \
        return (type) __res; \
    return (type) -1; \
}

#define _syscall2(type, name, type1, arg1, type2, arg2) \
type name(type1 arg1, type2 arg2) { \
    long __res; \
    __asm__ __volatile__ ( \
        "int $0x80" \
        : "=a" (__res) \
        : "0" (SYS_##name), "b" ((long)(arg1)), "c" ((long)(arg2)) \
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
        : "0" (SYS_##name), "b" ((long)(arg1)), "c" ((long)(arg2)), "d" ((long)(arg3)) \
        : "memory" \
    ); \
    if (__res >= 0) \
        return (type) __res; \
    return (type) -1; \
}

_syscall3(int32_t, read, int, fd, void*, buf, size_t, size);
_syscall3(int32_t, write, int, fd, const void*, buf, size_t, size);
_syscall3(int32_t, write1, int, fd, const void*, buf, size_t, size);
_syscall0(proc::pid_t, fork);
_syscall2(int32_t, open, const char*, path, int, flags);
_syscall1(int32_t, close, int, fd);


#include <core/sleep.h>
#include <fs/fs.h>

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
    int32_t ret_value = 0;
    switch (call_numer) {
        case SYS_read:
            ret_value = fs::sys_read(arg1, (void *)arg2, arg3);
            break;
        case SYS_write:
            ret_value = fs::sys_write(arg1, (const void*)arg2, arg3);
            break;
        case SYS_write1:
            ret_value = sys_write(arg1, (const void *)arg2, arg3);
            break;
        case SYS_fork:
            ret_value = proc::sys_fork(frame);
            break;
        case SYS_open:
            ret_value = fs::sys_open((const char*)arg1, arg2);
            break;
        case SYS_close:
            ret_value = fs::sys_close(arg1);
            break;
        default:
            break;
    }
    frame->gp.eax = ret_value;
}

void syscall_init() {
    iroute_register(IRQ_VECTOR_SYSTEM_CALLS, syscall_handler);
}
