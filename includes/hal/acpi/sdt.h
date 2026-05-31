#ifndef HAL_ACPI_SDT_H
#define HAL_ACPI_SDT_H

#include <stdint-gcc.h>

typedef struct {
    uint32_t signature;         // Table signature
    uint32_t length;            // Total length of table
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    uint64_t oem_table_id;
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct {
    acpi_sdt_header_t header;
    uint32_t entry[];
} __attribute__((packed)) acpi_rsdt_t;


#endif