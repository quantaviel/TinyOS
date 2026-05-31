#include <klib/string.h>

#include <stdint-gcc.h>

auto memcpy(void *dst, const void *src, size_t size) -> void* {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (size--) *d++ = *s++;
    return dst;
}

auto memcmp(const void *lhs, const void *rhs, size_t size) -> int {
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