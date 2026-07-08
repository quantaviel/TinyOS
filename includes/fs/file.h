#pragma once

#include <stdint-gcc.h>
#include <fs/inode.h>

#define O_RDONLY 0x1
#define O_WRONLY 0x2
#define O_RDWR   0x4
#define O_APPEND 0x8
#define O_CREAT  0x10

namespace fs {

struct file;

struct file_operations {
    int (*read) (file*, void*, size_t);
    int (*write)(file*, const void*, size_t);
};

struct file {
    uint32_t f_mode;
    uint32_t f_count;
    uint32_t f_pos;
    file_operations* f_ops;

    inode* f_inode;
};

extern file* default_tty;

file* file_alloc();
void file_free(file* f);
void file_init();
file* dup_file(file* f);

} // namespace fs
