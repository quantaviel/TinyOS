#include <fs/vfs.h>
#include <fs/buf.h>
#include <fs/tinyfs/tinyfs.h>

#include <klib/string.h>

namespace vfs {

void file_lock();
void file_unlock();

inode* root_inode {nullptr};

bool init() {
    if (root_inode != nullptr) {
        return default_tty != nullptr;
    }

    block_cache::init();
    super_block* root_fs = tinyfs::load(1);
    if (root_fs == nullptr) {
        return false;
    }

    static dentry root {.parent = nullptr, .in = root_fs->root, .name = "/"};
    if (mount(nullptr, root_fs, &root, nullptr) < 0) {
        return false;
    }

    inode* dev_dir = namei("/dev");
    if (dev_dir == nullptr) {
        dev_dir = root_inode->i_ops->mkdir(root_inode, "dev", 0755);
    }
    if (dev_dir == nullptr) {
        return false;
    }
    if (!S_ISDIR(dev_dir->i_mode)) {
        dev_dir->sb->iput(dev_dir);
        return false;
    }

    dev_dir->sb->iput(dev_dir);

    inode* tty_inode = namei("/dev/tty");
    if (tty_inode != nullptr) {
        bool is_character_device = S_ISCHR(tty_inode->i_mode);
        tty_inode->sb->iput(tty_inode);
        if (!is_character_device) {
            return false;
        }
    } else if (mknod("/dev/tty", S_IFCHR | 0666, tinyfs::TTY_DEVICE) < 0) {
        return false;
    }

    default_tty = open("/dev/tty", O_RDWR);
    return default_tty != nullptr;
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
        
        if (sb == nullptr) {
            /// FUTURE: Change to assert and abort
            return nullptr;
        }
        
        if (i_ops == nullptr || i_ops->lookup == nullptr) {
            curr_ip->sb->iput(curr_ip);
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

inode* parent_resolve(const char* path, char* filename) {
    if (path == nullptr || *path == '\0') {
        return nullptr;
    }

    int len = strlen(path);
    len = len - 1;
    if (path[len] == '/') {
        // not a valid path of a file
        return nullptr;
    }

    int cache_len = 0, flen = 0;
    char str_cache[100];
    while (path[len] != '/' && len) {
        str_cache[cache_len++] = path[len];
        --len;
    }
    
    cache_len--;
    while (cache_len >= 0) {
        filename[flen++] = str_cache[cache_len--];
    }
    filename[flen] = '\0';

    for (int i = 0; i <= len; ++i) {
        str_cache[i] = path[i];
    }
    if (len != 0) str_cache[len] = '\0';
    else str_cache[len + 1] = '\0';

    return namei(str_cache);
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

int mknod(const char* path, uint32_t mode, uint32_t dev) {
    if (auto in = namei(path); in != nullptr) {
        // file already exists
        in->sb->iput(in);
        return -1;
    }

    char filename[100];
    inode* parent = parent_resolve(path, filename);

    if (parent == nullptr) {
        return -1;
    }
    
    if (!S_ISDIR(parent->i_mode)) {
        // parent is not directory
        parent->sb->iput(parent);
        return -1;
    }

    if (parent->i_ops == nullptr || parent->i_ops->mknod == nullptr) {
        parent->sb->iput(parent);
        return -1;
    }

    int err = parent->i_ops->mknod(parent, filename, mode, dev);
    parent->sb->iput(parent);

    return err;
}

} // namespace vfs