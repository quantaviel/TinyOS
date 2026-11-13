#pragma once

#include <core/process.h>

#include <stdint-gcc.h>
#include <stddef.h>

#define SYS_restart 0
#define SYS_exit    1
#define SYS_fork    2
#define SYS_read    3
#define SYS_write   4
#define SYS_open    5
#define SYS_close   6

#define syscall0(type, name) \
type name();

#define syscall1(type, name, type1, arg1)\
type name(type1 arg1)

#define syscall2(type, name, type1, arg1, type2, arg2)\
type name(type1 arg1, type2 arg2)

#define syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)\
type name(type1 arg1, type2 arg2, type3 arg3);

syscall3(int, read, int, fd, void *, buf, size_t, count);
syscall3(int, write, int, fd, const void*, buf, size_t, size);
syscall0(proc::pid_t, fork);
syscall2(int32_t, open, const char*, path, int, flags);
syscall1(int32_t, close, int, fd);
syscall1(void, exit, int, status);

void syscall_init(void);