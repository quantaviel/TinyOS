#pragma once

#include <stdint-gcc.h>
#include <stddef.h>

extern "C" {

void* memcpy(void *dst, const void *src, size_t size);

int memcmp(const void *lhs, const void *rhs, size_t size);

void* memset(void *src, int v, size_t size);

char* strcpy(char *dst, const char* src);

size_t strlen(const char* src);

int strcmp(const char *lhs, const char *rhs);

}
