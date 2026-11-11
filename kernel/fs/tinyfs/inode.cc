#include <fs/tinyfs/inode.h>
#include <fs/buf.h>

namespace tinyfs {

namespace buf = block_cache;
constexpr uint32_t INODE_HASH_SIZE = 16;
constexpr uint32_t NINODE = 64;

struct inode_hash {
    list_node hash_head;
};

static inode_hash bucket[INODE_HASH_SIZE];
static inode inode_pool[NINODE];
static list_node inode_list_head;

void inode_init() {
    for (uint32_t i = 0; i < INODE_HASH_SIZE; ++i) {
        list_init(&bucket[i].hash_head);
    }

    list_init(&inode_list_head);
    for (uint32_t i = 0; i < NINODE; ++i) {
        inode_pool[i] = {};
        list_push_back(&inode_list_head, &inode_pool[i].inode_list);
    }
}

void inode_write(vfs::inode* ip) {
    if (!ip->is_dirty) {
        return;
    }

    fs_instance* fs = get_instance(ip->sb);
    uint32_t lba = fs->sb.inode_table_block + ip->i_number / inode_count_per_block;
    uint32_t offset = (ip->i_number % inode_count_per_block) * sizeof(d_inode);
    buf::buffer_head* bh = buf::read(fs->dev, lba);
    if (bh == nullptr) {
        return;
    }

    inode* private_ip = to_tinyfs_inode(ip);
    d_inode* disk_inode = reinterpret_cast<d_inode*>(bh->data + offset);
    disk_inode->i_mode = ip->i_mode;
    disk_inode->i_size = ip->i_size;
    disk_inode->i_nlinks = private_ip->i_nlinks;
    disk_inode->i_blocks = private_ip->i_blocks;
    disk_inode->i_mtime = private_ip->i_mtime;
    disk_inode->i_atime = private_ip->i_atime;
    disk_inode->i_rdev = private_ip->i_rdev;
    for (uint32_t i = 0; i < DIRECT_BLOCKS; ++i) {
        disk_inode->i_zone[i] = private_ip->i_zone[i];
    }

    bh->flags |= B_DIRTY;
    buf::release(bh);
    // 元数据已交给块缓存；ATA 写回失败时，缓存仍保留 B_DIRTY。
    ip->is_dirty = 0;
}

vfs::inode* iget(vfs::super_block* sb, uint32_t inode_number) {
    fs_instance* fs = get_instance(sb);
    if (inode_number >= fs->sb.inode_blocks * inode_count_per_block) {
        return nullptr;
    }

    uint32_t hash_id = inode_number & (INODE_HASH_SIZE - 1);
    list_node* iter;
    list_for_each(&bucket[hash_id].hash_head, iter) {
        inode* private_ip = container_of(iter, inode, hash_list);
        vfs::inode* ip = &private_ip->vfs_inode;
        if (ip->is_valid && ip->sb == sb && ip->i_number == inode_number) {
            ++ip->ref_count;
            return ip;
        }
    }

    inode* curr_ip = nullptr;
    list_for_each(&inode_list_head, iter) {
        // 空闲扫描必须使用 inode_list，而不是 hash_list 的成员偏移。
        inode* private_ip = container_of(iter, inode, inode_list);
        vfs::inode* ip = &private_ip->vfs_inode;
        if (ip->ref_count == 0 && !ip->is_dirty) {
            curr_ip = private_ip;
            break;
        }
    }
    if (curr_ip == nullptr) {
        return nullptr;
    }

    uint32_t lba = fs->sb.inode_table_block + inode_number / inode_count_per_block;
    uint32_t offset = (inode_number % inode_count_per_block) * sizeof(d_inode);
    buf::buffer_head* bh = buf::read(fs->dev, lba);
    if (bh == nullptr) {
        return nullptr;
    }

    // 保留 inode_list 的连接，只移除被复用对象的旧哈希连接。
    if (curr_ip->hash_list.next != nullptr) {
        list_remove(&curr_ip->hash_list);
    }

    vfs::inode* ip = &curr_ip->vfs_inode;
    *ip = {};
    ip->sb = sb;
    ip->i_number = inode_number;
    ip->ref_count = 1;

    d_inode* disk_inode = reinterpret_cast<d_inode*>(bh->data + offset);
    ip->i_mode = disk_inode->i_mode;
    ip->i_size = disk_inode->i_size;
    curr_ip->i_nlinks = disk_inode->i_nlinks;
    curr_ip->i_blocks = disk_inode->i_blocks;
    curr_ip->i_mtime = disk_inode->i_mtime;
    curr_ip->i_atime = disk_inode->i_atime;
    curr_ip->i_rdev = disk_inode->i_rdev;
    for (uint32_t i = 0; i < DIRECT_BLOCKS; ++i) {
        curr_ip->i_zone[i] = disk_inode->i_zone[i];
    }
    buf::release(bh);

    set_inode_operations(ip);
    ip->is_valid = 1;
    list_push_back(&bucket[hash_id].hash_head, &curr_ip->hash_list);
    return ip;
}

void iput(vfs::inode* ip) {
    if (ip == nullptr || ip->ref_count == 0) {
        return;
    }

    --ip->ref_count;
    // 根目录等对象长期持有引用，不能只在 ref_count == 0 时保存修改。
    inode_write(ip);
}

int inode_nr_alloc(vfs::super_block* sb) {
    fs_instance* fs = get_instance(sb);
    uint32_t inode_count = fs->sb.inode_blocks * inode_count_per_block;
    if (inode_count > bitmap_capacity) {
        inode_count = bitmap_capacity;
    }

    buf::buffer_head* bh = buf::read(fs->dev, fs->sb.inode_bitmap_block);
    if (bh == nullptr) {
        return -1;
    }

    // inode 0 是根 inode，分配从 1 开始；编号按每字节 8 位计算。
    for (uint32_t inode_number = 1; inode_number < inode_count; ++inode_number) {
        uint32_t byte = inode_number / 8;
        uint32_t bit = inode_number % 8;
        if (!(bh->data[byte] & (1u << bit))) {
            bh->data[byte] |= 1u << bit;
            bh->flags |= B_DIRTY;
            buf::release(bh);
            return inode_number;
        }
    }

    buf::release(bh);
    return -1;
}

} // namespace tinyfs
