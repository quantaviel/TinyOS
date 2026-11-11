#include <fs/tinyfs/dir.h>
#include <fs/tinyfs/inode.h>
#include <fs/tinyfs/block.h>
#include <fs/buf.h>
#include <klib/string.h>

namespace tinyfs {

namespace buf = block_cache;
constexpr uint32_t entries_per_block = BLOCK_SIZE / sizeof(directory);

bool valid_name(const char* name) {
    if (name == nullptr || *name == '\0') {
        return false;
    }

    size_t length = 0;
    while (name[length] != '\0') {
        if (name[length] == '/' || length >= NAME_MAX) {
            return false;
        }
        ++length;
    }
    return true;
}

int find_entry(vfs::inode* dir_inode, const char* name) {
    if (dir_inode == nullptr || !S_ISDIR(dir_inode->i_mode) || !valid_name(name)) {
        return -1;
    }

    fs_instance* fs = get_instance(dir_inode->sb);
    inode* private_ip = to_tinyfs_inode(dir_inode);
    size_t name_length = strlen(name);

    for (uint32_t i = 0; i < DIRECT_BLOCKS; ++i) {
        if (private_ip->i_zone[i] == 0) {
            continue;
        }

        buf::buffer_head* bh = buf::read(fs->dev, private_ip->i_zone[i]);
        if (bh == nullptr) {
            return -1;
        }

        directory* de = reinterpret_cast<directory*>(bh->data);
        for (uint32_t j = 0; j < entries_per_block; ++j) {
            uint32_t position = i * BLOCK_SIZE + j * sizeof(directory);
            if (position >= dir_inode->i_size) {
                break;
            }

            // 不能用 inode_number != 0 判断有效性，根目录编号就是 0。
            if (de[j].name[0] != '\0' &&
                memcmp(de[j].name, name, name_length) == 0 &&
                de[j].name[name_length] == '\0') {
                uint32_t inode_number = de[j].inode_number;
                buf::release(bh);
                return inode_number;
            }
        }
        buf::release(bh);
    }
    return -1;
}

int add_entry(vfs::inode* dir_inode, const char* name, uint32_t inode_number) {
    if (dir_inode == nullptr || !S_ISDIR(dir_inode->i_mode) || !valid_name(name)) {
        return -1;
    }

    fs_instance* fs = get_instance(dir_inode->sb);
    inode* private_ip = to_tinyfs_inode(dir_inode);

    for (uint32_t i = 0; i < DIRECT_BLOCKS; ++i) {
        if (private_ip->i_zone[i] == 0) {
            int block = block_alloc(dir_inode->sb);
            if (block < 0) {
                return -1;
            }

            buf::buffer_head* bh = buf::read(fs->dev, block);
            if (bh == nullptr) {
                bitmap_clear(fs->dev, fs->sb.data_bitmap_block, block);
                return -1;
            }
            memset(bh->data, 0, BLOCK_SIZE);
            bh->flags |= B_DIRTY;
            buf::release(bh);

            private_ip->i_zone[i] = block;
            ++private_ip->i_blocks;
            dir_inode->is_dirty = 1;
        }

        buf::buffer_head* bh = buf::read(fs->dev, private_ip->i_zone[i]);
        if (bh == nullptr) {
            return -1;
        }

        directory* de = reinterpret_cast<directory*>(bh->data);
        for (uint32_t j = 0; j < entries_per_block; ++j) {
            if (de[j].name[0] != '\0') {
                continue;
            }

            de[j].inode_number = inode_number;
            memset(de[j].name, 0, sizeof(de[j].name));
            memcpy(de[j].name, name, strlen(name));

            uint32_t end = i * BLOCK_SIZE + (j + 1) * sizeof(directory);
            if (end > dir_inode->i_size) {
                dir_inode->i_size = end;
            }
            dir_inode->is_dirty = 1;
            bh->flags |= B_DIRTY;
            buf::release(bh);
            return 0;
        }
        buf::release(bh);
    }
    return -1;
}

vfs::inode* create(vfs::inode* parent, const char* name, uint32_t mode, uint32_t device) {
    if (parent == nullptr || !S_ISDIR(parent->i_mode) || !valid_name(name) ||
        strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return nullptr;
    }
    if (find_entry(parent, name) >= 0) {
        return nullptr;
    }

    fs_instance* fs = get_instance(parent->sb);
    int inode_number = inode_nr_alloc(parent->sb);
    if (inode_number < 0) {
        return nullptr;
    }

    vfs::inode* ip = iget(parent->sb, inode_number);
    if (ip == nullptr) {
        bitmap_clear(fs->dev, fs->sb.inode_bitmap_block, inode_number);
        return nullptr;
    }

    // 新文件必须重置磁盘槽位原来的数据，但保留缓存链表及 VFS 身份。
    inode* private_ip = to_tinyfs_inode(ip);
    ip->i_mode = mode;
    ip->i_size = 0;
    ip->is_dirty = 1;
    private_ip->i_nlinks = S_ISDIR(mode) ? 2 : 1;
    private_ip->i_blocks = 0;
    private_ip->i_mtime = 0;
    private_ip->i_atime = 0;
    private_ip->i_rdev = device;
    memset(private_ip->i_zone, 0, sizeof(private_ip->i_zone));
    set_inode_operations(ip);

    bool created = true;
    if (S_ISDIR(mode)) {
        created = add_entry(ip, ".", inode_number) == 0;
        if (created) {
            created = add_entry(ip, "..", parent->i_number) == 0;
        }
    }
    if (created) {
        created = add_entry(parent, name, inode_number) == 0;
    }

    if (!created) {
        for (uint32_t i = 0; i < DIRECT_BLOCKS; ++i) {
            if (private_ip->i_zone[i] != 0) {
                bitmap_clear(fs->dev, fs->sb.data_bitmap_block, private_ip->i_zone[i]);
            }
        }
        bitmap_clear(fs->dev, fs->sb.inode_bitmap_block, inode_number);
        // 不写回失败的 inode。旧哈希连接在下次复用时移除。
        ip->ref_count = 0;
        ip->is_valid = 0;
        ip->is_dirty = 0;
        return nullptr;
    }

    if (S_ISDIR(mode)) {
        ++to_tinyfs_inode(parent)->i_nlinks;
        parent->is_dirty = 1;
    }
    inode_write(ip);
    inode_write(parent);
    return ip;
}

} // namespace tinyfs
