#pragma once

#include <stdint-gcc.h>
#include <stddef.h>

extern "C" {

auto memcpy(void *dst, const void *src, size_t size) -> void*;

auto memcmp(const void *lhs, const void *rhs, size_t size) -> int;

auto memset(void *src, int v, size_t size) -> void*;

auto strcpy(char *dst, const char* src) -> char*;

auto strlen(const char* src) -> size_t;

auto strcmp(const char *lhs, const char *rhs) -> int;

}
