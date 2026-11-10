#pragma once

#include <stdint-gcc.h>
#include <stddef.h>

namespace vfs {


/* ========================================================================== */
/* 文件类型掩码 (File Type Mask)                                                */
/* ========================================================================== */
#define S_IFMT   0170000  // 位掩码：用于剥离出高 4 位的文件类型

#define S_IFREG  0100000  // 普通文件 (Regular file) -> 0x8000
#define S_IFDIR  0040000  // 目录文件 (Directory)    -> 0x4000
#define S_IFCHR  0020000  // 字符设备 (Character dev)-> 0x2000
#define S_IFBLK  0060000  // 块设备 (Block device)   -> 0x6000

/* ========================================================================== */
/* 访问权限掩码 (Permission Bits)                                               */
/* ========================================================================== */
// 所有者权限 (User / Owner)
#define S_IRWXU  00700    // 所有者读写执行 (rwx------)
#define S_IRUSR  00400    // 所有者读       (r--------)
#define S_IWUSR  00200    // 所有者写       (-w-------)
#define S_IXUSR  00100    // 所有者执行     (--x------)

// 组用户权限 (Group)
#define S_IRWXG  00070    // 组用户读写执行 (---rwx---)
#define S_IRGRP  00040    // 组用户读       (---r-----)
#define S_IWGRP  00020    // 组用户写       (----w----)
#define S_IXGRP  00010    // 组用户执行     (-----x---)

// 其他人权限 (Other)
#define S_IRWXO  00007    // 其他人读写执行 (------rwx)
#define S_IROTH  00004    // 其他人读       (------r--)
#define S_IWOTH  00002    // 其他人写       (-------w-)
#define S_IXOTH  00001    // 其他人执行     (--------x)

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG) // 是否是普通文件
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR) // 是否是目录
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR) // 是否是字符设备（stdin/stdout）

#define O_RDONLY 0x1
#define O_WRONLY 0x2
#define O_RDWR   0x4
#define O_APPEND 0x8
#define O_CREAT  0x10

struct inode;

// super block

struct super_block {
    inode* root;
    void* data;

    // get inode by inode number in suber block (in fact, dev)
    inode* (*iget) (super_block*, uint32_t);

    // recyle inode and try to write back to disk
    void   (*iput) (inode*);

    // inode number alloc
    int    (*inode_nr_alloc) (super_block*);
};


// inode

struct inode;
struct file_operations;

struct inode_operations {
    // start from inode, find inode with name: inode pointer, name
    inode* (*lookup) (inode*, const char*);

    // create normal file under parent inode: inode pointer, name, mode
    inode* (*create) (inode*, const char*, uint32_t);
    
    // create directory under parent inode: inode pointer, name, mode
    inode* (*mkdir)  (inode*, const char*, uint32_t);
};

struct inode {
    uint32_t i_mode;
    uint32_t i_size;
    uint32_t ref_count;
    uint32_t i_number;
    inode_operations* i_ops;
    file_operations* f_ops;  // default file operations
    super_block* sb;

    uint8_t is_valid;
    uint8_t is_dirty;
};


// directory entry

struct dentry {
    inode* in;
    char name[60];
};


// file

struct file;

struct file_operations {
    int (*open) (file*);
    int (*close)(file*);
    int (*read) (file*, void*, size_t);
    int (*write)(file*, const void*, size_t);
};

enum class file_state {
    free,
    opening,
    opened,
    releasing,
};

struct file {
    uint32_t f_mode;
    uint32_t f_count;
    uint32_t f_pos;
    file_operations* f_ops;

    inode* f_inode;
    file_state f_state;
};

file* file_alloc(inode* f_inode);
void  file_free(file* f);
file* dup_file(file* f);


// mount

struct mount_entry {
    super_block* sb;
    dentry* root;
    mount_entry* parent;
    dentry* mount_point;
};

int mount(mount_entry* parent, super_block* sb, dentry* root, dentry* mount_point);

void init();
inode* namei(const char* path);

file* open(const char* path, int flags);
int close(file* f);
int read(file* f, void* buf, size_t count);
int write(file* f, const void* buf, size_t count);
inode* mkdir(dentry* dir, const char* name, int mode);

extern inode* root_inode;


} // namespace vfs