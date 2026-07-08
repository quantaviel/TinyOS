#pragma once

#include <fs/file.h>

namespace tty {

extern fs::file_operations tty_file_operations;

int write(fs::file* fp, const void* buf, size_t count);
int read(fs::file* fp, void* buf, size_t count);


} // namespace tty