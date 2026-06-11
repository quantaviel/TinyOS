#pragma once

#include <driver/ata.h>
#include <fs/inode.h>
#include <stddef.h>

#define FS_MAGIC_NUMBER 0x4F534653
#define FS_BLOCK_SIZE   512

namespace fs {

void init();

int sys_open(const char* path, int flags);

int sys_close(int fd);

int sys_read(int fd, void* buf, size_t count);

int sys_write(int fd, const void* buf, size_t count);

} // namespace fs
