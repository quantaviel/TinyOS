#pragma once

#include <fs/tinyfs/inode.h>
#include <fs/tinyfs/block.h>
#include <fs/tinyfs/dir.h>
#include <fs/tinyfs/file.h>

namespace tinyfs {

// 统一入口：加载后端实例、绑定 VFS 操作表、创建设备节点。
// 不承担路径解析、挂载或打开文件对象的生命周期管理。
vfs::super_block* load(uint32_t dev);
int mknod(vfs::inode* parent, const char* name, uint32_t mode, uint32_t device);

} // namespace tinyfs
