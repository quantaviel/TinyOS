#include <fs/tinyfs/tinyfs.h>
#include <fs/buf.h>
#include <hal/tty.h>
#include <klib/string.h>

namespace tinyfs {

namespace buf = block_cache;

static fs_instance root_fs;
static vfs::super_block root_super_block;
static bool loaded;

// 这些轻量适配函数只为 create 指定类型，实际目录处理在 dir.cc 中。
static vfs::inode* lookup(vfs::inode* parent, const char* name) {
    int inode_number = find_entry(parent, name);
    if (inode_number < 0) {
        return nullptr;
    }
    return iget(parent->sb, inode_number);
}

static vfs::inode* create_file(vfs::inode* parent, const char* name, uint32_t mode) {
    return create(parent, name, S_IFREG | (mode & 0777));
}

static vfs::inode* mkdir(vfs::inode* parent, const char* name, uint32_t mode) {
    return create(parent, name, S_IFDIR | (mode & 0777));
}

// 统一绑定后端目录、普通文件和设备操作。
static vfs::inode_operations directory_operations {
    .lookup = lookup,
    .create = create_file,
    .mkdir = mkdir,
    .mknod = mknod,
};

static vfs::file_operations regular_file_operations {
    .open = nullptr,
    .close = nullptr,
    .read = file_read,
    .write = file_write,
};

static vfs::file_operations directory_file_operations {};

void set_inode_operations(vfs::inode* ip) {
    ip->i_ops = nullptr;
    ip->f_ops = nullptr;

    if (S_ISDIR(ip->i_mode)) {
        ip->i_ops = &directory_operations;
        ip->f_ops = &directory_file_operations;
    } else if (S_ISREG(ip->i_mode)) {
        ip->f_ops = &regular_file_operations;
    } else if (S_ISCHR(ip->i_mode)) {
        inode* private_ip = to_tinyfs_inode(ip);
        if (private_ip->i_rdev == TTY_DEVICE) {
            ip->f_ops = &tty::tty_file_operations;
        }
    }
}

// 实例加载与设备节点入口。
int mknod(vfs::inode* parent, const char* name, uint32_t mode, uint32_t device) {
    if (!S_ISCHR(mode) || device != TTY_DEVICE) {
        return -1;
    }

    vfs::inode* ip = create(parent, name, S_IFCHR | (mode & 0777), device);
    if (ip == nullptr) {
        return -1;
    }

    iput(ip);
    return 0;
}

vfs::super_block* load(uint32_t dev) {
    // 当前 ATA 驱动只有一个磁盘入口，不能只凭 dev 参数假装支持多盘。
    if (dev != 1) {
        return nullptr;
    }
    if (loaded) {
        return &root_super_block;
    }

    buf::buffer_head* bh = buf::read(dev, 1);
    if (bh == nullptr) {
        return nullptr;
    }
    root_fs.dev = dev;
    memcpy(&root_fs.sb, bh->data, sizeof(super_block));
    buf::release(bh);

    const super_block& sb = root_fs.sb;
    if (sb.magic_number != MAGIC || sb.inode_blocks == 0 ||
        sb.inode_blocks > bitmap_capacity / inode_count_per_block ||
        sb.inode_table_block < 2 || sb.inode_table_block >= sb.total_blocks ||
        sb.inode_blocks > sb.total_blocks - sb.inode_table_block ||
        sb.data_start_block < sb.inode_table_block + sb.inode_blocks ||
        sb.data_start_block >= sb.total_blocks ||
        sb.inode_bitmap_block >= sb.inode_table_block ||
        sb.data_bitmap_block >= sb.inode_table_block) {
        return nullptr;
    }

    inode_init();
    root_super_block = {
        .root = nullptr,
        .data = &root_fs,
        .iget = iget,
        .iput = iput,
        .inode_nr_alloc = inode_nr_alloc,
    };
    root_super_block.root = iget(&root_super_block, 0);
    if (root_super_block.root == nullptr) {
        return nullptr;
    }
    if (!S_ISDIR(root_super_block.root->i_mode)) {
        iput(root_super_block.root);
        root_super_block.root = nullptr;
        return nullptr;
    }

    loaded = true;
    return &root_super_block;
}

} // namespace tinyfs
