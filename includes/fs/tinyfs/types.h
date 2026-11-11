#pragma once

#include <ds/list.h>
#include <fs/vfs.h>

namespace tinyfs {

constexpr size_t NAME_MAX = 59;
constexpr uint32_t BLOCK_SIZE = 512;
constexpr uint32_t MAGIC = 0x4F534653;
constexpr uint32_t TTY_DEVICE = 1;
constexpr uint32_t DIRECT_BLOCKS = 12;

// 原 fs.h / inode.h：磁盘布局，与 mkfs.py 保持一致。
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

#pragma pack(push, 1)
struct d_inode {
    uint16_t i_mode;
    uint16_t i_nlinks;
    uint32_t i_size;
    uint32_t i_blocks;
    uint32_t i_mtime;
    uint32_t i_atime;
    uint32_t i_zone[DIRECT_BLOCKS];

    // 从原来的 60 字节 padding 中取 4 字节保存设备标识，不改变磁盘尺寸。
    uint32_t i_rdev;
    uint8_t i_padding[56];
};

// 原 dir.h：inode 0 也是合法编号，空目录项以 name[0] == '\0' 表示。
struct directory {
    uint32_t inode_number;
    char name[NAME_MAX + 1];
};
#pragma pack(pop)

static_assert(sizeof(super_block) == 32);
static_assert(sizeof(d_inode) == 128);
static_assert(sizeof(directory) == 64);

// 原 inode：通用属性由 VFS 持有，tinyfs 只增加自身需要的数据。
struct inode {
    vfs::inode vfs_inode;

    uint16_t i_nlinks;
    uint32_t i_blocks;
    uint32_t i_mtime;
    uint32_t i_atime;
    uint32_t i_zone[DIRECT_BLOCKS];
    uint32_t i_rdev;

    list_node hash_list;
    list_node inode_list;
};

static_assert(offsetof(inode, vfs_inode) == 0);

// super_block::data 指向的后端实例，供各子模块访问所属设备与磁盘布局。
struct fs_instance {
    uint32_t dev;
    super_block sb;
};

constexpr uint32_t inode_count_per_block = BLOCK_SIZE / sizeof(d_inode);
constexpr uint32_t bitmap_capacity = BLOCK_SIZE * 8;

inline inode* to_tinyfs_inode(vfs::inode* ip) {
    // vfs_inode 是 tinyfs::inode 的首成员。
    return reinterpret_cast<inode*>(ip);
}

inline fs_instance* get_instance(vfs::super_block* sb) {
    return static_cast<fs_instance*>(sb->data);
}

} // namespace tinyfs
