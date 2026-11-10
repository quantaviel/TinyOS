#include <fs/vfs.h>

namespace vfs {

void file_lock();
void file_unlock();

inode* root_inode = nullptr;

void init() {

}

int mount(mount_entry* parent, super_block* sb, dentry* root, dentry* mount_point) {

}

inode* namei(const char* path) {
    if (path == nullptr || *path == '\0') return nullptr;
    
    inode* curr_ip;
    if (path[0] == '/') {
        curr_ip = root_inode->sb->iget(root_inode->sb, root_inode->i_number);
        if (curr_ip == nullptr) return nullptr;
        path++;
    } else {
        /// TODO: support current working directory
        return nullptr;
    }

    #define NAMEBUF_LEN 100
    char name_buf[NAMEBUF_LEN];
    
    while (*path) {
        int len = 0;
        while (*path != '/' && *path != '\0' && len < NAMEBUF_LEN) {
            name_buf[len++] = *path;
            ++path;
        }
        
        bool next_is_dir = false;
        while (*path == '/') {
            next_is_dir = true;
            ++path;
        }
        if (len == 0) continue; // resolve '//'
        
        auto sb = curr_ip->sb;
        auto i_ops = curr_ip->i_ops;
        
        if (sb == nullptr || i_ops == nullptr || i_ops->lookup == nullptr) {
            /// FUTURE: Change to assert
            return nullptr;
        }
        
        if (len == NAMEBUF_LEN) {
            sb->iput(curr_ip);
            return nullptr;
        }
        name_buf[len] = '\0';

        if (!S_ISDIR(curr_ip->i_mode)) {
            sb->iput(curr_ip);
            return nullptr;
        }

        auto next_ip = i_ops->lookup(curr_ip, name_buf);
        if (next_ip == nullptr) {
            sb->iput(curr_ip);
            return nullptr;
        }

        if (next_is_dir && !S_ISDIR(next_ip->i_mode)) {
            sb->iput(curr_ip);
            next_ip->sb->iput(next_ip);
            return nullptr;
        }

        sb->iput(curr_ip);
        curr_ip = next_ip;
    }

    return curr_ip;
}

file* open(const char* path, int flags) {
    inode* in = namei(path);
    if (in == nullptr) {
        return nullptr;
    }

    file* f = file_alloc(in);
    if (f == nullptr) {
        in->sb->iput(in);
        return nullptr;
    }
    f->f_mode = flags;
    
    if (f->f_ops == nullptr) {
        file_free(f);
        return nullptr;
    }

    if (f->f_ops->open) {
        int err = f->f_ops->open(f);
        if (err < 0) {
            file_free(f);
            return nullptr;
        }
    }

    file_lock();
    f->f_state = file_state::opened;
    file_unlock();

    return f;
}

int close(file* f) {
    if (f == nullptr) {
        return -1;
    }

    file_free(f);
    return 1;
}

int read(file* f, void* buf, size_t count) {
    if (f == nullptr) {
        return -1;
    }

    if (f->f_ops == nullptr || f->f_ops->read == nullptr) {
        return -1;
    }

    return f->f_ops->read(f, buf, count);
}

int write(file* f, const void* buf, size_t count) {
    if (f == nullptr) {
        return -1;
    }

    if (f->f_ops == nullptr || f->f_ops->write == nullptr) {
        return -2;
    }

    return f->f_ops->write(f, buf, count);
}

inode* mkdir(dentry* dir, const char* name, int mode) {
    if (dir == nullptr || name == nullptr || dir->in == nullptr) {
        return nullptr;
    }

    inode* in = dir->in;

    if (in->i_ops == nullptr || in->i_ops->mkdir == nullptr) {
        return nullptr;
    }

    return dir->in->i_ops->mkdir(dir->in, name, mode);
}

} // namespace vfs