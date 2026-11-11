#include <fs/tinyfs/file.h>
#include <fs/tinyfs/inode.h>
#include <fs/tinyfs/block.h>
#include <fs/buf.h>
#include <klib/string.h>

namespace tinyfs {

namespace buf = block_cache;
constexpr uint32_t max_file_size = DIRECT_BLOCKS * BLOCK_SIZE;

int file_read(vfs::file* fp, void* data, size_t count) {
    vfs::inode* ip = fp->f_inode;
    inode* private_ip = to_tinyfs_inode(ip);
    fs_instance* fs = get_instance(ip->sb);

    if (fp->f_pos >= ip->i_size) {
        return 0;
    }
    if (count > ip->i_size - fp->f_pos) {
        count = ip->i_size - fp->f_pos;
    }

    size_t bytes_read = 0;
    while (bytes_read < count) {
        uint32_t pos = fp->f_pos;
        uint32_t logical_block = pos / BLOCK_SIZE;
        uint32_t offset = pos % BLOCK_SIZE;
        if (logical_block >= DIRECT_BLOCKS) {
            break;
        }

        size_t chunk_size = BLOCK_SIZE - offset;
        if (chunk_size > count - bytes_read) {
            chunk_size = count - bytes_read;
        }

        uint32_t lba = private_ip->i_zone[logical_block];
        if (lba == 0) {
            memset(static_cast<char*>(data) + bytes_read, 0, chunk_size);
        } else {
            buf::buffer_head* bh = buf::read(fs->dev, lba);
            if (bh == nullptr) {
                return bytes_read != 0 ? static_cast<int>(bytes_read) : -1;
            }
            memcpy(static_cast<char*>(data) + bytes_read, bh->data + offset, chunk_size);
            buf::release(bh);
        }

        bytes_read += chunk_size;
        fp->f_pos += chunk_size;
    }
    return bytes_read;
}

int file_write(vfs::file* fp, const void* data, size_t count) {
    vfs::inode* ip = fp->f_inode;
    inode* private_ip = to_tinyfs_inode(ip);
    fs_instance* fs = get_instance(ip->sb);
    uint32_t pos = fp->f_pos;
    if (fp->f_mode & O_APPEND) {
        pos = ip->i_size;
    }

    if (pos >= max_file_size) {
        return count != 0 ? -1 : 0;
    }
    if (count > max_file_size - pos) {
        count = max_file_size - pos;
    }

    size_t bytes_written = 0;
    while (bytes_written < count) {
        uint32_t logical_block = pos / BLOCK_SIZE;
        uint32_t offset = pos % BLOCK_SIZE;
        size_t chunk_size = BLOCK_SIZE - offset;
        if (chunk_size > count - bytes_written) {
            chunk_size = count - bytes_written;
        }

        uint32_t lba = private_ip->i_zone[logical_block];
        if (lba == 0) {
            int block = block_alloc(ip->sb);
            if (block < 0) {
                break;
            }

            buf::buffer_head* bh = buf::read(fs->dev, block);
            if (bh == nullptr) {
                bitmap_clear(fs->dev, fs->sb.data_bitmap_block, block);
                break;
            }
            memset(bh->data, 0, BLOCK_SIZE);
            bh->flags |= B_DIRTY;
            buf::release(bh);

            private_ip->i_zone[logical_block] = block;
            ++private_ip->i_blocks;
            lba = block;
        }

        buf::buffer_head* bh = buf::read(fs->dev, lba);
        if (bh == nullptr) {
            break;
        }
        memcpy(bh->data + offset, static_cast<const char*>(data) + bytes_written, chunk_size);
        bh->flags |= B_DIRTY;
        buf::release(bh);

        pos += chunk_size;
        bytes_written += chunk_size;
        fp->f_pos = pos;
        if (pos > ip->i_size) {
            ip->i_size = pos;
        }
        ip->is_dirty = 1;
    }

    inode_write(ip);
    if (bytes_written == 0 && count != 0) {
        return -1;
    }
    return bytes_written;
}

} // namespace tinyfs
