#include <fs/vfs.h>
#include <core/process.h>

namespace vfs {

static file* file_for_fd(int fd) {
    auto* process = proc::current_pcb;
    if (process == nullptr || fd < 0 || fd >= MAX_FD) {
        return nullptr;
    }

    return process->fd_array[fd];
}

int sys_open(const char* path, int flags) {
    auto* process = proc::current_pcb;
    if (process == nullptr) {
        return -1;
    }

    int fd = 0;
    while (fd < MAX_FD && process->fd_array[fd] != nullptr) {
        ++fd;
    }
    if (fd == MAX_FD) {
        return -1;
    }

    file* opened_file = open(path, flags);
    if (opened_file == nullptr) {
        return -1;
    }

    process->fd_array[fd] = opened_file;
    return fd;
}

int sys_close(int fd) {
    file* opened_file = file_for_fd(fd);
    if (opened_file == nullptr) {
        return -1;
    }

    proc::current_pcb->fd_array[fd] = nullptr;
    return close(opened_file);
}

int sys_read(int fd, void* buf, size_t count) {
    return read(file_for_fd(fd), buf, count);
}

int sys_write(int fd, const void* buf, size_t count) {
    return write(file_for_fd(fd), buf, count);
}

} // namespace vfs
