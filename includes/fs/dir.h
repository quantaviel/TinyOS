#pragma once

#include <stdint-gcc.h>

#include <fs/inode.h>

namespace fs {

#pragma pack(push, 1)

struct directory {
    uint32_t inode_number;
    char name[60];
};

#pragma pack(pop)

// find inode_number in dir_inode by dir name
int find_entry(inode* dir_inode, const char* name);

// resolve path
inode* namei(const char* path);

int mkdir(const char* path, uint32_t flags);



} // namespace fs
