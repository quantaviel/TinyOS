#include <hal/vga.h>
#include <hal/tty.h>
#include <fs/vfs.h>

namespace tty {

vfs::file_operations tty_file_operations {nullptr, nullptr, read, write};

int write(vfs::file* fp, const void* buf, size_t count) {
    vfs::inode* ip = fp->f_inode;
    if (!S_ISCHR(ip->i_mode)) {
        return -1;
    }
    for (size_t i = 0; i < count; ++i) {
        const char* str = static_cast<const char*>(buf) + i;
        vga::putchar(*str);
    }
    return count;
}

int read(vfs::file*, void*, size_t) {
    // Keyboard input is not yet connected to the TTY.
    return -1;
}

} // namespace tty
