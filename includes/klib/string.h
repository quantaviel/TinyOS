#pragma once

#include <stddef.h>

auto memcpy(void *src, const void *dst, size_t size) -> void*;

auto memcmp(const void *lhs, const void *rhs, size_t size) -> int;