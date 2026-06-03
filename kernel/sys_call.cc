#include <arch/x86/int.h>
#include <arch/x86/iroute.h>

#include <sys_call.h>
#include <proc.h>

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

// _syscall0(proc::pid_t, fork);
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
_syscall3(int32_t, write, int, fd, const char*, buf, size_t, size);

int32_t sys_write(int fd, const char* buf, size_t size) {
    for (int i = 0; i < size; ++i) {
        vga::putchar(buf[i]);
    }
    return size;
}

void syscall_handler(int_frame* frame) {
    uint32_t call_numer = frame->gp.eax;
    uint32_t arg1 = frame->gp.ebx;
    uint32_t arg2 = frame->gp.ecx;
    uint32_t arg3 = frame->gp.edx;
    switch (call_numer) {
        case 2:
            proc::sys_fork(frame);
            break;
        case 4:
            sys_write(arg1, (const char*) arg2, arg3);
            break;
        default:
            break;
    }
}

void syscall_init() {
    iroute_reg(IRQ_VECTOR_SYSTEM_CALLS, syscall_handler);
}
