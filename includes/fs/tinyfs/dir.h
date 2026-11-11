#pragma once

#include <fs/tinyfs/types.h>

namespace tinyfs {

// 单个目录内部的名称检查、查找和插入，不解析完整路径。
bool valid_name(const char* name);
int find_entry(vfs::inode* dir_inode, const char* name);
int add_entry(vfs::inode* dir_inode, const char* name, uint32_t inode_number);
vfs::inode* create(vfs::inode* parent, const char* name, uint32_t mode,
                   uint32_t device = 0);

} // namespace tinyfs
