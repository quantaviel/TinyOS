#include <fs/inode.h>
#include <fs/fs.h>
#include <fs/buf.h>
#include <ds/list.h>

#define INODE_HASH_SIZE 16
#define NINODE 64 // 内存中最多同时活跃 64 个 inode

namespace fs {

struct inode_hash {
    list_node hash_head;
    // lock
};
static inode_hash bucket[INODE_HASH_SIZE];

inode inode_pool[NINODE];
list_node inode_list_head;

super_block sb{0};

constexpr uint32_t inode_count_per_block = 512 / sizeof(d_inode);

inode* iget(uint32_t dev, uint32_t inode_number) {
    uint32_t hash_id = inode_number & (INODE_HASH_SIZE - 1);
    list_node* iter;
    inode* curr_ip = nullptr;
    // lock
    list_for_each(&bucket[hash_id].hash_head, iter) {
        inode* ip = container_of(iter, inode, hash_list);
        if (ip->dev == dev && ip->inode_number == inode_number) {
            ip->ref_count++;
            // unlock
            return ip;
        }
    }

    list_for_each(&inode_list_head, iter) {
        inode* ip = container_of(iter, inode, hash_list);
        if (ip->ref_count == 0) {
            list_remove(&ip->hash_list);
            curr_ip = ip;
            break;
        }
    }
    
    curr_ip->dev = dev;
    curr_ip->inode_number = inode_number;
    curr_ip->ref_count = 1;
    curr_ip->is_valid = 0;
    list_push_back(&bucket[hash_id].hash_head, &curr_ip->hash_list);

    // read inode from disk
    uint32_t lba = sb.inode_table_block + inode_number / inode_count_per_block;
    uint32_t offset = (inode_number % inode_count_per_block) * sizeof(d_inode);
    
    buf::buffer_head* buf = buf::read(dev, lba);
    d_inode* disk_inode = (d_inode*)(buf->data + offset);
    
    curr_ip->i_mode = disk_inode->i_mode;
    curr_ip->i_size = disk_inode->i_size;
    curr_ip->i_blocks = disk_inode->i_blocks;
    curr_ip->i_atime = disk_inode->i_atime;
    curr_ip->i_mtime = disk_inode->i_mtime;
    curr_ip->i_nlinks = disk_inode->i_nlinks;
    for (int i = 0; i < 11; ++i)
        curr_ip->i_zone[i] = disk_inode->i_zone[i];
    
    buf::release(buf);
    
    return curr_ip;
}

void iput(inode* ip) {
    // lock
    ip->ref_count--;
    if (ip->ref_count == 0 && ip->is_dirty) {
        // write to block
        uint32_t lba = sb.inode_table_block + ip->inode_number / inode_count_per_block;
        uint32_t offset = ip->inode_number % inode_count_per_block * sizeof(d_inode);
        
        buf::buffer_head* bh = buf::read(ip->dev, lba);
        d_inode* di = (d_inode*)(bh->data + offset);

        di->i_size = ip->i_size;
        di->i_blocks = ip->i_blocks;
        di->i_nlinks = ip->i_nlinks;
        di->i_mode = ip->i_mode;
        di->i_atime = ip->i_atime;
        di->i_mtime = ip->i_mtime;
        di->i_size = ip->i_size;
        for (int i = 0; i < 11; ++i) {
            di->i_zone[i] = ip->i_zone[i];
        }

        bh->flags |= B_DIRTY;
        buf::release(bh);

        // for (int i = 0; i < 11; ++i) {
        //     if (ip->i_zone[i] != 0) {
        //         buf::buffer_head* bh = buf::getblk(ip->dev, ip->i_zone[i]);
        //         if (bh == nullptr) continue;
        //         buf::write_to_disk(bh);
        //         buf::release(bh);
        //     }
        // }
    }
}

int32_t inode_nr_alloc(uint32_t dev) {
    buf::buffer_head* bh = buf::read(dev, sb.inode_bitmap_block);
    uint8_t* inode_bitmap = (uint8_t*)bh->data;
    for (int i = 0; i < FS_BLOCK_SIZE; ++i) {
        if (inode_bitmap[i] != 0xFF) {
            uint32_t bit = __builtin_ctz(~inode_bitmap[i]);
            inode_bitmap[i] |= (1 << bit);

            bh->flags |= B_DIRTY;
            buf::release(bh);
            
            return i << 8 | bit;
        }
    }
    // no available inode number
    buf::release(bh);
    return -1;
}

void inode_init() {
    for (int i = 0; i < INODE_HASH_SIZE; ++i) {
        list_init(&bucket[i].hash_head);
    }
    list_init(&inode_list_head);
    for (int i = 0; i < NINODE; ++i) {
        list_push_back(&inode_list_head, &inode_pool[i].inode_list);
    }
}

} // namespace fs
