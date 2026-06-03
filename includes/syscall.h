#pragma once

#include <process.h>

#include <stdint-gcc.h>
#include <stddef.h>

#define __NR_read  0
#define __NR_write 1
#define __NR_fork  2

#define syscall0(type, name) \
type name();

#define syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)\
type name(type1 arg1, type2 arg2, type3 arg3)

syscall3(int32_t, read, int, fd, void *, buf, size_t, count);
syscall3(int32_t, write, int, fd, const void*, buf, size_t, size);
syscall0(proc::pid_t, fork);

void syscall_init(void);