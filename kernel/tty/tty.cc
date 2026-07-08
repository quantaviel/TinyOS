#include <hal/vga.h>
#include <hal/tty.h>
#include <fs/file.h>

namespace tty {

fs::file_operations tty_file_operations {.read = read, .write = write};

int write(fs::file* fp, const void* buf, size_t count) {
    fs::inode* ip = fp->f_inode;
    if (S_ISCHR(ip->i_mode)) {
        // Error: Not a char file
        return 0;
    }
    for (int i = 0; i < count; ++i) {
        const char* str = static_cast<const char*>(buf) + i;
        vga::putchar(*str);
    }
    return count;
}

int read(fs::file* fp, void* buf, size_t count) {
    
}

} // namespace tty
