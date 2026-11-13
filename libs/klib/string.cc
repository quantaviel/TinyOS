#include <klib/string.h>
#include <klib/utils.h>

#include <stdint-gcc.h>

void* memcpy(void *dst, const void *src, size_t size) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (size--) *d++ = *s++;
    return dst;
}

int memcmp(const void *lhs, const void *rhs, size_t size) {
    const uint8_t* l = (const uint8_t *)lhs;
    const uint8_t* r = (const uint8_t *)rhs;
    while (size--) {
        if (*l != *r) {
            if (*l > *r) return 1;
            else return -1;
        }
        l++;
        r++;
    }
    return 0;
}

void* memset(void *src, int v, size_t size) {
    uint8_t* d = (uint8_t*) src;
    while (size--) *d++ = (uint8_t)v;
    return src;
}

char* strcpy(char *dst, const char* src) {
    char *d = dst;
    while (*src) *dst++ = *src++;
    *dst = '\0';
    return d;
}

size_t strlen(const char* src) {
    uint32_t len = 0;
    while (*src) ++src, ++len;
    return len;
}

int strcmp(const char *lhs, const char *rhs) {
    while (*lhs && *rhs) {
        if (*lhs != *rhs) {
            if (*lhs > *rhs) return 1;
            else return -1;
        }
        ++lhs;
        ++rhs;
    }

    if (*lhs == '\0' && *rhs != '\0') return -1;
    if (*rhs == '\0' && *lhs != '\0') return 1;
    return 0;
}
