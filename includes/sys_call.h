#pragma once

#include <proc.h>

#include <stdint-gcc.h>
#include <stddef.h>

#define __NR_fork 2
#define __NR_write 4

#define syscall0(type, name) \
type name();

#define syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)\
type name(type1 arg1, type2 arg2, type3 arg3)

syscall0(proc::pid_t, fork);
syscall3(int32_t, write, int, fd, const char*, buf, size_t, size);

void syscall_init();