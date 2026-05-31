#ifndef MADT_PARSER_H
#define MADT_PARSER_H

#include <stddef.h>

#include <hal/acpi/acpi.h>

int
madt_parser(acpi_madt_t *madt, acpi_ctx_t *ctx);

#endif