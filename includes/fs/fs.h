#pragma once

#include <driver/ata.h>
#include <fs/inode.h>
#include <stddef.h>

#define FS_MAGIC_NUMBER 0x4F534653
#define FS_BLOCK_SIZE   512
#define STDIN_FNO    0
#define STDOUT_FNO   1
#define STDERR_FNO   2

namespace fs {

void init();
uint32_t block_alloc(uint32_t dev);
int create_at(const char* path, int mode);

int sys_open(const char* path, int flags);

int sys_close(int fd);

int sys_read(int fd, void* buf, size_t count);

int sys_write(int fd, const void* buf, size_t count);

int sys_mkdir(const char* path, int mode);

} // namespace fs
