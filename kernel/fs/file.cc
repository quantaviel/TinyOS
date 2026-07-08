#include <fs/dir.h>
#include <fs/file.h>
#include <fs/fs.h>

#include <hal/tty.h>

namespace fs {

#define NFILES 128

file file_table[NFILES];
file* default_tty = nullptr;
// global lock

file* file_alloc() {
    // lock
    for (int i = 0; i < NFILES; ++i) {
        if (file_table[i].f_count == 0) {
            file_table[i].f_count = 1;
            file_table[i].f_pos = 0;
            file_table[i].f_inode = nullptr;
            file_table[i].f_ops = nullptr;

            // unlock
            return &file_table[i];
        }
    }

    // unlock
    return nullptr;
}

void file_free(file* f) {
    // lock
    f->f_count--;
    if (f->f_count == 0) {
        if (f->f_inode != nullptr) {
            iput(f->f_inode);
            f->f_inode = nullptr;
        }
    }
    // unlock
}

file* dup_file(file* f) {
    if (f == nullptr) return nullptr;

    // lock
    f->f_count++;
    // unlock

    return f;
}

void file_init() {
    default_tty = file_alloc();
    default_tty->f_inode = namei("/dev/tty");
    default_tty->f_mode = O_RDWR;
    default_tty->f_ops = &tty::tty_file_operations;
}

} // namespace fs
