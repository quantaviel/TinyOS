#pragma once

#include <stdint-gcc.h>

#include <ds/list.h>

#define HASH_SIZE 16
#define NBUF 64

#define B_VALID 0x1
#define B_DIRTY 0x2

namespace block_cache {

constexpr uint32_t BLOCK_SIZE = 512;

struct buffer_head {
    uint8_t* data;
    uint32_t block_number;
    uint16_t dev;

    uint32_t flags;
    uint32_t ref_count;

    list_node hash_list;
    list_node buf_list;
};

struct hash_bucket {
    list_node hash_head;
    // mutex lock
};

buffer_head* read(uint32_t dev, uint32_t lba);
buffer_head* getblk(uint32_t dev, uint32_t lba);
void write_to_disk(buffer_head* bh);
void write(uint32_t dev, uint32_t lba, void* data);
void release(buffer_head* buf);
void init();

} // namespace block_cache
