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
    
    int entry_count = dir_inode->i_size / sizeof(directory);

    fmt::println("dir_inode number: {}, dir_inode nlinks: {}", dir_inode->inode_number, dir_inode->i_nlinks);

    for (int i = 0; i < 11; ++i) {
        if (dir_inode->i_zone[i] == 0) break;

        buf::buffer_head* bh = buf::read(dir_inode->dev, dir_inode->i_zone[i]);        
        directory* de = (directory*)bh->data;

        for (int j = 0; j < FS_BLOCK_SIZE / sizeof(directory); ++j) {
            // fmt::println("directory name: {s}, inode_nr: {}, cmp: {}", de[j].name, de[j].inode_number, strcmp(de[j].name, name));
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

} // namespace fs
