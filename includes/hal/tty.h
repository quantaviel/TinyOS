#pragma once

#include <fs/vfs.h>

namespace tty {

extern vfs::file_operations tty_file_operations;

int write(vfs::file* fp, const void* buf, size_t count);
int read(vfs::file* fp, void* buf, size_t count);


} // namespace tty