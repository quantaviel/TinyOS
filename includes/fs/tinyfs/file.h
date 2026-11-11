#pragma once

#include <fs/tinyfs/types.h>

namespace tinyfs {

// 普通文件的数据操作；fd 和打开文件对象仍由 VFS 管理。
int file_read(vfs::file* fp, void* data, size_t count);
int file_write(vfs::file* fp, const void* data, size_t count);

} // namespace tinyfs
