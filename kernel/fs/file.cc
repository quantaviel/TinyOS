#include <fs/file.h>

namespace fs {

#define NFILES 128

file file_table[NFILES];
// global lock

file* file_alloc() {
    // lock
    for (int i = 0; i < NFILES; ++i) {
        if (file_table[i].f_count == 0) {
            file_table[i].f_count = 1;
            file_table[i].f_pos = 0;
            file_table[i].f_inode = nullptr;

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
}

} // namespace fs
