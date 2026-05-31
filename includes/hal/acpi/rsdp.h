#ifndef HAL_ACPI_RSDP
#define HAL_ACPI_RSDP

#include <stdint-gcc.h>

typedef struct {
    char     signature[8];      // "RSD PTR "
    uint8_t  checksum;          // ACPI 1.0 checksum
    char     oem_id[6];
    uint8_t  revision;          // 0 = ACPI 1.0, 2 = ACPI 2.0+
    uint32_t rsdt_addr;         // 32-bit physical address of RSDT

    /* Valid only if revision >= 2 */
    uint32_t length;            // Total size of RSDP
    uint64_t xsdt_addr;         // 64-bit physical address of XSDT
    uint8_t  ext_checksum;      // Checksum of entire RSDP
    uint8_t  reserved[3];
} __attribute__((packed)) acpi_rsdp_t;


#endif