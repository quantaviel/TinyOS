#pragma once

#include <stdint-gcc.h>

#include <ds/list.h>

/* ========================================================================== */
/* 💾 1. 文件类型掩码 (File Type Mask)                                        */
/* ========================================================================== */
#define S_IFMT   0170000  // 位掩码：用于剥离出高 4 位的文件类型

#define S_IFREG  0100000  // 普通文件 (Regular file) -> 0x8000
#define S_IFDIR  0040000  // 目录文件 (Directory)    -> 0x4000
#define S_IFCHR  0020000  // 字符设备 (Character dev)-> 0x2000
#define S_IFBLK  0060000  // 块设备 (Block device)   -> 0x6000

/* ========================================================================== */
/* 🔒 2. 访问权限掩码 (Permission Bits)                                       */
/* ========================================================================== */
// 所有者权限 (User / Owner)
#define S_IRWXU  00700    // 所有者读写执行 (rwx------)
#define S_IRUSR  00400    // 所有者读       (r--------)
#define S_IWUSR  00200    // 所有者写       (-w-------)
#define S_IXUSR  00100    // 所有者执行     (--x------)

// 组用户权限 (Group)
#define S_IRWXG  00070    // 组用户读写执行 (---rwx---)
#define S_IRGRP  00040    // 组用户读       (---r-----)
#define S_IWGRP  00020    // 组用户写       (----w----)
#define S_IXGRP  00010    // 组用户执行     (-----x---)

// 其他人权限 (Other)
#define S_IRWXO  00007    // 其他人读写执行 (------rwx)
#define S_IROTH  00004    // 其他人读       (------r--)
#define S_IWOTH  00002    // 其他人写       (-------w-)
#define S_IXOTH  00001    // 其他人执行     (--------x)

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG) // 是否是普通文件
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR) // 是否是目录
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR) // 是否是字符设备（stdin/stdout）

namespace fs {

struct inode {
    // in disk:
    uint16_t i_mode;
    uint16_t i_nlinks;
    uint32_t i_size;
    uint32_t i_blocks;
    uint32_t i_mtime; /// TODO: time_t
    uint32_t i_atime; /// TODO: time_t
    uint32_t i_zone[12];

    // in os:
    uint8_t dev;
    uint32_t ref_count;
    uint32_t inode_number;
    uint8_t is_valid;
    uint8_t is_dirty;
    // lock
    list_node hash_list;
    list_node inode_list;
};

#pragma pack(push, 1)

struct d_inode {
    uint16_t i_mode;
    uint16_t i_nlinks;
    uint32_t i_size;
    uint32_t i_blocks;
    uint32_t i_mtime; /// TODO: time_t
    uint32_t i_atime; /// TODO: time_t
    uint32_t i_zone[12];
};

#pragma pack(pop)

struct super_block {
    uint32_t magic_number;
    uint32_t total_blocks;
    uint32_t inode_blocks;
    uint32_t data_blocks;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_block;
    uint32_t data_start_block;
};

inode* iget(uint32_t dev, uint32_t inode_number);
void iput(inode* ip);
void inode_init();

} // namespace fs