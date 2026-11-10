#include <fs/vfs.h>

namespace vfs {

#define NFILES 128

file file_table[NFILES];
file* default_tty = nullptr;
/// TODO: Add global lock

void file_lock() {}

void file_unlock() {}

file* file_alloc(inode* f_inode) {
    file_lock();
    for (int i = 0; i < NFILES; ++i) {
        if (file_table[i].f_state == file_state::free) {
            file_table[i].f_count = 1;
            file_table[i].f_pos = 0;
            file_table[i].f_inode = f_inode;
            file_table[i].f_ops = f_inode->f_ops;
            file_table[i].f_state = file_state::opening;

            file_unlock();
            return &file_table[i];
        }
    }

    file_unlock();
    return nullptr;
}

void file_free(file* f) {
    if (f == nullptr) return ;
    
    file_lock();
    
    if (f->f_count == 0) {
        file_unlock();
        return;
    }
    if (f->f_count > 1) {
        f->f_count--;
        file_unlock();
        return;
    }

    bool opened = (f->f_state == file_state::opened);
    auto op = f->f_ops;
    auto in = f->f_inode;
    f->f_state = file_state::releasing;
    f->f_count = 0;
    file_unlock();

    if (opened && op && op->close) {
        op->close(f);
    }
    
    if (in) {
        in->sb->iput(in);
    }

    file_lock();
    f->f_count = 0;
    f->f_inode = nullptr;
    f->f_ops   = nullptr;
    f->f_mode  = 0;
    f->f_state = file_state::free;
    file_unlock();
    
    return;
}

file* dup_file(file* f) {
    if (f == nullptr) return nullptr;

    file_lock();
    if (f->f_state != file_state::opened) {
        file_unlock();
        return nullptr;
    }
    f->f_count++;
    file_unlock();

    return f;
}

} // namespace vfs