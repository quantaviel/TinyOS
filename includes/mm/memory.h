#pragma once

#include <stdint-gcc.h>

namespace mm {
    
typedef unsigned long vaddr_t;
typedef unsigned long paddr_t;
    
constexpr unsigned long ADDR_MAX = 0xFFFFFFFF;
constexpr unsigned long PAGE_MAX = ADDR_MAX >> 12;


} // namespace mm

