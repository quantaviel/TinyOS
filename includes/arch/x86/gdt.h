#pragma once
#include <stdint-gcc.h>

/*
    According to page 2908, the definition of Segment Descriptor.
*/

// 注意小端序
struct gdt_desc_t {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags_limit;
    uint8_t  base_high;
} __attribute__((packed));

struct gdtr_t {
    uint16_t limit;    // GDT大小-1
    uint32_t base;     // GDT基地址
} __attribute__((packed));

/*
SD_TYPE:
  X => eXecute
  A => Accessed
  E => Expand-down
  C => Conforming
*/
#define SD_TYPE_DATA_READ_ONLY               (0x0 << 8)
#define SD_TYPE_DATA_READ_ONLY_A             (0x1 << 8)
#define SD_TYPE_DATA_READ_WRITE              (0x2 << 8)
#define SD_TYPE_DATA_READ_WRITE_A            (0x3 << 8)
#define SD_TYPE_DATA_READ_ONLY_E             (0x4 << 8)
#define SD_TYPE_DATA_READ_ONLY_E_A           (0x5 << 8)
#define SD_TYPE_DATA_READ_WRITE_E            (0x6 << 8)
#define SD_TYPE_DATA_READ_WRITE_E_A          (0x7 << 8)
#define SD_TYPE_CODE_X_ONLY                  (0x8 << 8)
#define SD_TYPE_CODE_X_ONLY_A                (0x9 << 8)
#define SD_TYPE_CODE_X_READ                  (0xA << 8)
#define SD_TYPE_CODE_X_READ_A                (0xB << 8)
#define SD_TYPE_CODE_X_ONLY_C                (0xC << 8)
#define SD_TYPE_CODE_X_ONLY_C_A              (0xD << 8)
#define SD_TYPE_CODE_X_READ_C                (0xE << 8)
#define SD_TYPE_CODE_X_READ_C_A              (0xF << 8)

/* When System */
#define SD_TYPE_SYS_TSS_AVAILABLE            (0x9 << 8)

#define SD_DESCRIPTOR_TYPE_SYSTEM            0          // default
#define SD_DESCRIPTOR_TYPE_CODE_OR_DATA      (1 << 12)

#define SD_RING0                            (0x0 << 13)
#define SD_RING1                            (0x1 << 13)
#define SD_RING2                            (0x2 << 13)
#define SD_RING3                            (0x3 << 13)

#define SD_PRESENT                          (1 << 15)

/*SD_AVL*/

#define SD_64BIT                            (1 << 21)
#define SD_32BIT                            (1 << 22)
#define SD_4K_GRAN                          (1 << 23)

#define SD_BASE_H(x)            ((x) & 0xff000000)
#define SD_BASE_M(x)            (((x) & 0x00ff0000) >> 16)
#define SD_BASE_L(x)            (((x) & 0x0000ffff) << 16)

#define SD_LIMIT_H(x)           ((x) & 0xf0000)
#define SD_LIMIT_L(x)           ((x) & 0x0ffff)

#define SD_R0_CODE \
    SD_TYPE_CODE_X_READ | SD_DESCRIPTOR_TYPE_CODE_OR_DATA | \
    SD_RING0 | SD_PRESENT | SD_32BIT| SD_4K_GRAN

#define SD_R0_DATA \
    SD_TYPE_DATA_READ_WRITE | SD_DESCRIPTOR_TYPE_CODE_OR_DATA | \
    SD_RING0 | SD_PRESENT | SD_32BIT | SD_4K_GRAN

#define SD_R3_CODE \
    SD_TYPE_CODE_X_READ | SD_DESCRIPTOR_TYPE_CODE_OR_DATA | \
    SD_RING3 | SD_PRESENT | SD_32BIT | SD_4K_GRAN

#define SD_R3_DATA \
    SD_TYPE_DATA_READ_WRITE | SD_DESCRIPTOR_TYPE_CODE_OR_DATA | \
    SD_RING3 | SD_PRESENT | SD_32BIT | SD_4K_GRAN

#define SD_TSS \
    SD_TYPE_SYS_TSS_AVAILABLE | SD_DESCRIPTOR_TYPE_SYSTEM | \
    SD_RING0 | SD_PRESENT | SD_32BIT

#define SEGMENT_R0_CODE (1 * 8 | 0)
#define SEGMENT_R0_DATA (2 * 8 | 0)
#define SEGMENT_R3_CODE (3 * 8 | 3)
#define SEGMENT_R3_DATA (4 * 8 | 3)
