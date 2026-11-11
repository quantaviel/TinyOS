#include <fs/tinyfs/block.h>
#include <fs/buf.h>

namespace tinyfs {

namespace buf = block_cache;

int block_alloc(vfs::super_block* sb) {
    fs_instance* fs = get_instance(sb);
    uint32_t block_count = fs->sb.total_blocks;
    if (block_count > bitmap_capacity) {
        block_count = bitmap_capacity;
    }

    buf::buffer_head* bh = buf::read(fs->dev, fs->sb.data_bitmap_block);
    if (bh == nullptr) {
        return -1;
    }

    for (uint32_t block = fs->sb.data_start_block; block < block_count; ++block) {
        uint32_t byte = block / 8;
        uint32_t bit = block % 8;
        if (!(bh->data[byte] & (1u << bit))) {
            bh->data[byte] |= 1u << bit;
            bh->flags |= B_DIRTY;
            buf::release(bh);
            return block;
        }
    }

    buf::release(bh);
    return -1;
}

// 创建失败时撤销位图分配；不承担删除已有文件的语义。
void bitmap_clear(uint32_t dev, uint32_t bitmap_block, uint32_t number) {
    buf::buffer_head* bh = buf::read(dev, bitmap_block);
    if (bh == nullptr) {
        return;
    }

    bh->data[number / 8] &= ~(1u << (number % 8));
    bh->flags |= B_DIRTY;
    buf::release(bh);
}

} // namespace tinyfs
