#pragma once

#include <klib/fmt.h>

#define assert(expr) do {\
    if (!(expr)) {\
        fmt::println("Assertion Failed on {}:{}", __FILE__, __LINE__);\
        while (1); \
    } \
} while (0);

