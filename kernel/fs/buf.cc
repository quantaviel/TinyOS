#include <fs/buf.h>
#include <fs/fs.h>
#include <driver/ata.h>
#include <klib/string.h>
#include <klib/fmt.h>

namespace fs {

namespace buf {

uint8_t buf_data[NBUF][512];
buffer_head buf_pool[NBUF];
list_node buffer_list_head;

hash_bucket bucket[HASH_SIZE];

buffer_head* getblk(uint32_t dev, uint32_t lba);

buffer_head* read(uint32_t dev, uint32_t lba) {
    buffer_head* bh = getblk(dev, lba);
    if (!(bh->flags & B_VALID)) {
        // lock
        ata::ide_read_sectors(lba, 1, bh->data);
        bh->flags |= B_VALID;
        // unlock
    }
    return bh;
}

void write(uint32_t dev, uint32_t lba, void* data) {
    buffer_head* bh = getblk(dev, lba);
    // lock
    memcpy(bh->data, data, FS_BLOCK_SIZE);
    bh->flags |= B_DIRTY;
    // unlock
}

void write_to_disk(buffer_head* bh) {
    if (!(bh->flags & B_DIRTY)) return;
    // lock
    ata::ide_write_sector(bh->block_number, 1, bh->data);
    bh->flags &= ~B_DIRTY;
    // unlock
}

void release(buffer_head* buf) {
    uint32_t hash_id = buf->block_number & (HASH_SIZE - 1);
    // lock
    buf->ref_count--;
    if (buf->ref_count == 0) {
        list_remove(&buf->buf_list);
        list_push_front(&buffer_list_head, &buf->buf_list);
    }
    // unlock
}

buffer_head* getblk(uint32_t dev, uint32_t lba) {
    uint32_t hash_id = lba & (HASH_SIZE - 1);
    list_node* iter;
    // lock
    list_for_each(&bucket[hash_id].hash_head, iter) {
        buffer_head* buf = container_of(iter, buffer_head, hash_list);
        if (buf->block_number == lba) {
            // unlock
            buf->ref_count++;
            return buf;
        }
    }
    // unlock

    list_for_each(&buffer_list_head, iter) {
        buffer_head* buf = container_of(iter, buffer_head, buf_list);
        if (buf->ref_count == 0) {
            if (buf->flags & B_DIRTY) {
                // write to disk
                write_to_disk(buf);
                buf->flags &= ~B_DIRTY;
            }
            if (buf->flags & B_VALID) {
                list_remove(&buf->hash_list);
                buf->flags &= ~B_VALID;
            }
            buf->dev = dev;
            buf->block_number = lba;
            buf->flags = 0;
            buf->ref_count = 1;
            list_push_back(&bucket[hash_id].hash_head, &buf->hash_list);
            return buf;
        }
    }

    return nullptr;
}

void init() {
    for (int i = 0; i < NBUF; ++i) {
        buf_pool[i].data = buf_data[i];
    }
    for (int i = 0; i < HASH_SIZE; ++i) {
        list_init(&bucket[i].hash_head);
    }
    list_init(&buffer_list_head);
    for (int i = 0; i < NBUF; ++i) {
        list_push_front(&buffer_list_head, &buf_pool[i].buf_list);
    }
    fmt::println("Buffer init finished.");
}


} // namespace buf

} // namespace fs
