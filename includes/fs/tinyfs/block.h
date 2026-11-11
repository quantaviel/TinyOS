#pragma once

#include <fs/tinyfs/types.h>

namespace tinyfs {

// 数据块分配，以及创建失败时的位图回滚。
int block_alloc(vfs::super_block* sb);
void bitmap_clear(uint32_t dev, uint32_t bitmap_block, uint32_t number);

} // namespace tinyfs
