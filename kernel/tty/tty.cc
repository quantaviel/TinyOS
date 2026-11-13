#include <hal/keyboard.h>
#include <hal/vga.h>
#include <hal/tty.h>
#include <fs/vfs.h>

namespace tty {

static sleep::waiting_queue read_waiters;
static circular_queue<char, 512> input_queue;

void receive_char(char ch) {
    if (!input_queue.push(ch)) {
        return;
    }

    vga::putchar(ch);
    sleep::wake_up_one(&read_waiters);
}

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

int read(vfs::file* fp, void* buf, size_t count) {
    auto ip = fp->f_inode;
    if (!S_ISCHR(ip->i_mode)) {
        return -1;
    }
    if (count == 0) {
        return 0;
    }
    for (size_t i = 0; i < count; ++i) {
        uint8_t scancode = 0;
        while (!keyboard::read(scancode)) {
            sleep::sleep_event(keyboard::get_waiting_queue());
        }
        if (scancode > 0x7f) {
            --i;
            continue;
        }
        char* str = static_cast<char*>(buf) + i;
        *str = vga::recieve(keyboard::map(scancode));
    }
    return count;
}

} // namespace tty
