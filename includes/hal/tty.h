#pragma once

#include <fs/vfs.h>
#include <hal/vga.h>
#include <ds/queue.h>
#include <core/sleep.h>

namespace tty {

extern vfs::file_operations tty_file_operations;
void receive_char(char ch);

int write(vfs::file* fp, const void* buf, size_t count);
int read(vfs::file* fp, void* buf, size_t count);

} // namespace tty