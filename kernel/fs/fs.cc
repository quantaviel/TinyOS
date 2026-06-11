#include <fs/buf.h>
#include <fs/dir.h>
#include <fs/fs.h>
#include <fs/file.h>

#include <klib/fmt.h>
#include <klib/string.h>

#include <core/process.h>

namespace fs {

super_block sb;
inode* root_inode;
uint32_t block_alloc(uint32_t dev);

void init() {
    buf::init();
    buf::buffer_head* head = buf::read(1, 1);
    sb = *(super_block*)head->data;
    if (sb.magic_number != FS_MAGIC_NUMBER) {
        fmt::println("Error: Do not match magic number!");
        return;
    }
    inode_init();
    root_inode = iget(1, 0);
    fmt::println("root inode: i_mode: {x}, i_size: {}, i_mtime: {}, i_nlinks: {}", root_inode->i_mode, root_inode->i_size, root_inode->i_mtime, root_inode->i_nlinks);
}

int sys_open(const char* path, int flags) {
    inode* ip = namei(path);
    if (ip == nullptr) {
        return -1; // file not exist
    }
    
    file* fp = file_alloc();
    if (fp == nullptr) {
        return -2; // too many files
    }

    fp->f_inode = ip;
    fp->f_mode = flags;
    fp->f_pos = 0;
    fp->f_count = 1;

    int fd = -1;
    proc::proc* current_pcb = proc::get_current_proc();
    for (int i = 0; i < 16; ++i) {
        if (current_pcb->fd_array[i] == 0) {
            fd = i;
            current_pcb->fd_array[i] = fp;
            break;
        }
    }

    if (fd == -1) {
        file_free(fp);
        iput(ip);
        return -3; // too many fd
    }

    return fd;
}

int sys_close(int fd) {
    proc::proc* current_pcb = proc::get_current_proc();
    file* fp = current_pcb->fd_array[fd];
    if (fp == nullptr) {
        return -1; // unknow fd
    }

    file_free(fp);
    current_pcb->fd_array[fd] = 0;
    return 1;
}

int sys_read(int fd, void* buf, size_t count) {
    proc::proc* current_pcb = proc::get_current_proc();
    file* fp = current_pcb->fd_array[fd];
    if (fp == nullptr) {
        return -1; // unknow fd
    }

    inode* ip = fp->f_inode;
    uint32_t pos = fp->f_pos;

    if (pos >= ip->i_size) {
        return 0; // EOF
    }

    if (pos + count >= ip->i_size) {
        count = ip->i_size - pos;
    }

    uint32_t bytes_read = 0;

    while (bytes_read < count) {
        uint32_t logical_block = (pos + bytes_read) / FS_BLOCK_SIZE;
        uint32_t offset = (pos + bytes_read) % FS_BLOCK_SIZE;
        uint32_t remain = FS_BLOCK_SIZE - offset;
        uint32_t chunk_size = remain;

        if (remain > count - bytes_read) {
            chunk_size = count - bytes_read;
        }

        uint32_t lba = ip->i_zone[logical_block];
        
        buf::buffer_head* bh = buf::read(ip->dev, lba);
        
        memcpy(buf, bh->data + offset, chunk_size);
        
        buf += chunk_size;
        bytes_read += chunk_size;
        
        buf::release(bh);
    }

    fp->f_pos += bytes_read;

    return bytes_read;
}

int sys_write(int fd, const void* buf, size_t count) {
    proc::proc* current_pcb = proc::get_current_proc();
    file* fp = current_pcb->fd_array[fd];
    if (fp == nullptr) {
        return -1; // unknow fd
    }
    
    uint32_t pos = fp->f_pos;
    inode* ip = fp->f_inode;

    // ip lock

    if (fp->f_mode & O_APPEND) {
        pos = ip->i_size;
    }

    size_t bytes_written = 0;

    while (bytes_written < count) {
        uint32_t logical_block = pos / FS_BLOCK_SIZE;
        uint32_t offset = pos % FS_BLOCK_SIZE;

        uint32_t lba = ip->i_zone[logical_block];
        if (ip->i_zone[logical_block] == 0) {
            ip->i_zone[logical_block] = block_alloc(ip->dev);
            lba = ip->i_zone[logical_block];
            ip->i_blocks++;
        }

        uint32_t chunk_size = FS_BLOCK_SIZE - offset;
        
        if (chunk_size > count - bytes_written) {
            chunk_size = count - bytes_written;
        }

        buf::buffer_head* bh = buf::read(ip->dev, lba);
        
        memcpy(bh->data + offset, buf, chunk_size);

        bh->flags |= B_DIRTY;
        buf::release(bh);

        buf += chunk_size;
        bytes_written += chunk_size;
        pos += chunk_size;
    }

    fp->f_pos += bytes_written;
    if (fp->f_pos > ip->i_size) {
        ip->i_size = fp->f_pos;
    }

    ip->is_dirty = 1;

    // ip unlock

    return bytes_written;
}

uint32_t block_alloc(uint32_t dev) {
    buf::buffer_head* bh = buf::read(dev, sb.data_bitmap_block);
    uint8_t* data_bitmap = (uint8_t*)bh->data;
    uint32_t lba = -1;
    for (int i = 0; i < FS_BLOCK_SIZE; ++i) {
        if (data_bitmap[i] != 0xFF) {
            uint32_t mask = ~data_bitmap[i];
            mask = mask & (-mask);
            uint32_t bit = __builtin_ctz(mask);
            lba = (i << 3) | bit;

            data_bitmap[i] |= (1 << bit);
            break;
        }
    }
    buf::release(bh);
    return lba;
}

} // namespace fs
