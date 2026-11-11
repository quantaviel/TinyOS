#pragma once

#include <fs/tinyfs/types.h>

namespace tinyfs {

// inode 缓存与引用管理；成功获取的 inode 带一个引用。
void inode_init();
vfs::inode* iget(vfs::super_block* sb, uint32_t inode_number);
void iput(vfs::inode* ip);
void inode_write(vfs::inode* ip);
int inode_nr_alloc(vfs::super_block* sb);

// 操作表统一在 tinyfs.cc 中选择，加载和创建 inode 时调用。
void set_inode_operations(vfs::inode* ip);

} // namespace tinyfs
