#include <fs/buf.h>
#include <fs/dir.h>
#include <fs/fs.h>
#include <fs/inode.h>

#include <klib/string.h>
#include <klib/fmt.h>

namespace fs {

extern inode* root_inode;

int find_entry(inode* dir_inode, const char* name) {
    if (!(dir_inode->i_mode & S_IFDIR)) return -1;

    for (int i = 0; i < 11; ++i) {
        if (dir_inode->i_zone[i] == 0) break;

        buf::buffer_head* bh = buf::read(dir_inode->dev, dir_inode->i_zone[i]);        
        directory* de = (directory*)bh->data;

        for (int j = 0; j < FS_BLOCK_SIZE / sizeof(directory); ++j) {
            if (de[j].inode_number != 0 && strcmp(de[j].name, name) == 0) {
                uint32_t res_ir = de[j].inode_number;
                buf::release(bh);
                return res_ir;
            }
        }
        buf::release(bh);
    }
    return -1;
}

int add_entry(inode* dir_inode, const char* name, uint32_t sub_inode_nr) {
    if (!(dir_inode->i_mode & S_IFDIR)) return -1;

    for (int i = 0; i < 11; ++i) {
        if (dir_inode->i_zone[i] == 0) {
            uint32_t blk = block_alloc(dir_inode->dev);
            if (blk == -1) return -1;
            dir_inode->i_zone[i] = blk;
            dir_inode->i_blocks++;
            dir_inode->is_dirty = 1;
            buf::buffer_head* bh = buf::read(dir_inode->dev, blk);
            memset(bh->data, 0, FS_BLOCK_SIZE);
            buf::release(bh);
        }

        buf::buffer_head* bh = buf::read(dir_inode->dev, dir_inode->i_zone[i]);        
        directory* de = (directory*)bh->data;

        for (int j = 0; j < FS_BLOCK_SIZE / sizeof(directory); ++j) {
            if (de[j].inode_number == 0) {
                de[j].inode_number = sub_inode_nr;
                int name_len = strlen(name);
                /// TODO: length check
                for (int k = 0; k <= name_len; ++k) {
                    de[j].name[k] = name[k];
                }

                uint32_t cur_pos = i * FS_BLOCK_SIZE + (j + 1) * sizeof(directory);
                if (cur_pos > dir_inode->i_size) {
                    dir_inode->i_size = cur_pos;
                    dir_inode->is_dirty = 1;
                }

                bh->flags |= B_DIRTY;
                buf::release(bh);
                return 0;
            }
        }
        buf::release(bh);
    }
    return -1;
}

inode* namei(const char* path) {
    if (path == nullptr || *path == '\0') return nullptr;
    
    inode* curr_ip;
    if (path[0] == '/') {
        curr_ip = root_inode;
        curr_ip->ref_count++;
        path++;
    } else {
        /// TODO: support current working directory
        return nullptr;
    }

    char name_buf[100];

    while (*path) {
        int len = 0;
        while (*path != '/' && *path != '\0') {
            name_buf[len++] = *path;
            ++path;
        }

        if (*path == '/') ++path;
        if (len == 0) continue; // resolve '//'

        name_buf[len] = '\0';
        int res_ir = find_entry(curr_ip, name_buf);
        if (res_ir < 0) { // not find
            iput(curr_ip);
            return nullptr;
        }
        inode* next_ip = iget(curr_ip->dev, res_ir);
        iput(curr_ip);
        curr_ip = next_ip;
    }

    return curr_ip;
}

inode* dirnamei(const char* path, char* name) {
   if (path == nullptr || *path == '\0') return nullptr;
    
    inode* curr_ip;
    if (path[0] == '/') {
        curr_ip = root_inode;
        curr_ip->ref_count++;
        path++;
    } else {
        /// TODO: support current working directory
        return nullptr;
    }

    char name_buf[100];

    while (*path) {
        int len = 0;
        while (*path != '/' && *path != '\0') {
            name_buf[len++] = *path;
            ++path;
        }

        if (*path == '/') ++path;
        if (len == 0) continue; // resolve '//'

        name_buf[len] = '\0';
        if (*path == '\0') {
            for (int i = 0; i <= len; ++i) {
                name[i] = name_buf[i];
            }
            break;
        }
        int res_ir = find_entry(curr_ip, name_buf);
        if (res_ir < 0) { // not find
            iput(curr_ip);
            return nullptr;
        }
        inode* next_ip = iget(curr_ip->dev, res_ir);
        iput(curr_ip);
        curr_ip = next_ip;
    }

    return curr_ip;
}

inode* create(inode* parent, const char* name, uint32_t mode) {
    int32_t inode_nr = find_entry(parent, name);
    if (inode_nr >= 0) {
        // already exist
        return iget(parent->dev, inode_nr);
    }

    // create new file
    inode_nr = inode_nr_alloc(parent->dev);
    inode* ip = iget(parent->dev, inode_nr);
    ip->i_mode = mode;
    ip->i_size = 0;
    ip->i_blocks = 0;
    ip->i_atime = 0;
    ip->i_mtime = 0;
    /// TODO: current time
    ip->i_nlinks = 1;

    if (add_entry(parent, name, inode_nr) < 0) {
        /// TODO: roll back
        return nullptr;
    }

    if (S_ISDIR(mode)) {
        // create "." and ".." directory (self and parent)
        add_entry(ip, ".", inode_nr);
        add_entry(ip, "..", parent->inode_number);
        parent->i_nlinks++;
        ip->i_nlinks++;
    }
    ip->is_dirty = 1;
    parent->is_dirty = 1;
    return ip;
}

} // namespace fs
