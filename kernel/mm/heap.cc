#include <mm/heap.h>

/*

Implicit Free List Allocator

   [prologue] [chunk] ... [chunk] [epilogue]
   ^                               ^
   |                               |
 head                             end

prologue: 4 bytes of allocated memory to avoid edge cases
epilogue: 4 bytes of allocated memory to avoid edge cases

Every chunk:

  -------------------------------------------------
  | Size 4 bytes | ... Payload ... | Size 4 bytes |
  -------------------------------------------------
    ^              ^                 ^
    |              |                 |
    Header        Payload           Footer

Header:
    ------------------------------------------
    |         Size and Alloc Flag            |
    ------------------------------------------
    31                         2      1      0
    +--------------------------+------+------+
    |        Size (bytes)      |  pf  |  cf  |
    +--------------------------+------+------+
    pf: previous chunk used flag
    cf: current chunk used flag
    size: size of this chunk including header and footer

Footer(same as header):
    ------------------------------------------
    |         Size and Alloc Flag            |
    ------------------------------------------
    31                         2      1      0
    +--------------------------+------+------+
    |        Size (bytes)      |  pf  |  cf  |
    +--------------------------+------+------+
    pf: previous chunk used flag
    cf: current chunk used flag
    size: size of this chunk including header and footer

Size should be aligned to 4 bytes.

*/

#define EPILOGUE (0x4 | 0x1)
#define PROLOGUE (0x1)

#define CHUNK(size, pf, cf) ((size) | ((pf) << 1) | (cf))
#define CHUNK_SIZE(chunk) ((chunk) & ~0x3)
#define CHUNK_IS_USED(chunk) ((chunk) & 0x1)
#define CHUNK_PREV_USED(chunk) (((chunk) >> 1) & 0x1)

#define CHUNK_HEAD(chunk_addr) (*(uint32_t *)(chunk_addr))
#define CHUNK_TAIL(chunk_addr) (*(uint32_t *)((uintptr_t)(chunk_addr) + CHUNK_SIZE(CHUNK_HEAD(chunk_addr)) - WSIZE))

namespace mm {

void kheap::init(uintptr_t head, uintptr_t limit) {
    vmm::alloc_page(head);
    this->head = head;
    this->end = head + PAGE_SIZE - WSIZE;
    this->limit = limit;

    uint32_t *header = (uint32_t *)this->head;
    uint32_t *footer = (uint32_t *)this->end;
    *header = EPILOGUE;
    *footer = PROLOGUE;

    uint32_t *first_chunk_header = (uint32_t *)(this->head + WSIZE);
    uint32_t first_chunk_size = PAGE_SIZE - 2 * WSIZE;
    uint32_t *first_chunk_footer = (uint32_t *)(this->end - WSIZE);
    *first_chunk_header = CHUNK(first_chunk_size, 1, 0);
    *first_chunk_footer = CHUNK(first_chunk_size, 1, 0);
}

uintptr_t kheap::kalloc(size_t size) {
    size = (size + 3) & ~0x3; // align to 4 bytes
    size_t total_size = size + 2 * WSIZE; // include header and footer
    uintptr_t curr = this->head + WSIZE;
    while (curr < this->end) {
        uint32_t chunk_header = CHUNK_HEAD(curr);
        size_t chunk_size = CHUNK_SIZE(chunk_header);
        bool chunk_used = CHUNK_IS_USED(chunk_header);
        if (!chunk_used && chunk_size >= total_size) {
            // found a free chunk
            break;
        }
        curr += chunk_size;
    }
    if (curr >= this->end && !extend(total_size)) return 0;
    uint32_t chunk_header = CHUNK_HEAD(curr);
    size_t chunk_size = CHUNK_SIZE(chunk_header);
    bool chunk_used = CHUNK_IS_USED(chunk_header);
    // found a free chunk
    size_t remaining_size = chunk_size - total_size;
    if (remaining_size >= 2 * WSIZE + 4) {
        // split chunk
        uint32_t *new_chunk_header = (uint32_t *)(curr + total_size);
        uint32_t *new_chunk_footer = (uint32_t *)(curr + chunk_size - WSIZE);
        *new_chunk_header = CHUNK(remaining_size, 1, 0);
        *new_chunk_footer = CHUNK(remaining_size, 1, 0);
        // update allocated chunk
        uint32_t *alloc_chunk_header = (uint32_t *)curr;
        uint32_t *alloc_chunk_footer = (uint32_t *)(curr + total_size - WSIZE);
        *alloc_chunk_header = CHUNK(total_size, CHUNK_PREV_USED(chunk_header), 1);
        *alloc_chunk_footer = CHUNK(total_size, CHUNK_PREV_USED(chunk_header), 1);
    } else {
        // update next chunk's prev_used flag
        uintptr_t next_chunk_addr = curr + total_size;
        if (next_chunk_addr < this->end) {
            uint32_t *next_chunk_header = (uint32_t *)next_chunk_addr;
            *next_chunk_header |= 0x2; // set prev_used flag
            uint32_t *next_chunk_footer = (uint32_t *)(next_chunk_addr + CHUNK_SIZE(*next_chunk_header) - WSIZE);
            *next_chunk_footer |= 0x2; // set prev_used flag
        }
    }
    return curr + WSIZE;
}

void kheap::kfree(void *addr) {
    uintptr_t chunk_addr = (uintptr_t)addr - WSIZE;
    uint32_t *chunk_header = (uint32_t *)chunk_addr;
    size_t chunk_size = CHUNK_SIZE(*chunk_header);

    // mark chunk as free
    uint32_t *chunk_footer = (uint32_t *)(chunk_addr + chunk_size - WSIZE);
    *chunk_header = CHUNK(chunk_size, CHUNK_PREV_USED(*chunk_header), 0);
    *chunk_footer = CHUNK(chunk_size, CHUNK_PREV_USED(*chunk_header), 0);

    // update next chunk's prev_used flag
    uintptr_t next_chunk_addr = chunk_addr + chunk_size;
    if (next_chunk_addr < this->end) {
        uint32_t *next_chunk_header = (uint32_t *)next_chunk_addr;
        *next_chunk_header &= ~0x2; // clear prev_used flag
        uint32_t *next_chunk_footer = (uint32_t *)(next_chunk_addr + CHUNK_SIZE(*next_chunk_header) - WSIZE);
        *next_chunk_footer &= ~0x2; // clear prev_used flag
    }

    // merge with adjacent free chunks
    merge(chunk_addr);
}

bool kheap::extend(size_t size) {
    size_t extend_size = ((size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    if (this->end + extend_size + WSIZE > this->limit) return false;

    uintptr_t old_end = this->end;
    uint32_t old_epilogue = *(uint32_t *)(old_end);
    for (size_t offset = 0; offset < extend_size; offset += PAGE_SIZE) {
        vmm::alloc_page(this->end + WSIZE + offset);
    }
    this->end += extend_size;

    // create new free chunk
    uint32_t *new_chunk_header = (uint32_t *)(old_end);
    uint32_t new_chunk_size = extend_size + WSIZE; // include old epilogue
    uint32_t *new_chunk_footer = (uint32_t *)(this->end - WSIZE);
    *new_chunk_header = CHUNK(new_chunk_size, CHUNK_PREV_USED(old_epilogue), 0);
    *new_chunk_footer = CHUNK(new_chunk_size, CHUNK_PREV_USED(old_epilogue), 0);

    // update old epilogue to new chunk header
    *(uint32_t *)(old_end - WSIZE) = *new_chunk_header;

    // create new epilogue
    uint32_t *epilogue = (uint32_t *)(this->end);
    *epilogue = EPILOGUE;

    return true;
}

void kheap::merge(uintptr_t chunk_addr) {
    uint32_t chunk_header = CHUNK_HEAD(chunk_addr);
    size_t chunk_size = CHUNK_SIZE(chunk_header);

    // merge with next chunk if free
    uintptr_t next_chunk_addr = chunk_addr + chunk_size;
    if (next_chunk_addr < this->end) {
        uint32_t next_chunk_header = CHUNK_HEAD(next_chunk_addr);
        bool next_chunk_used = CHUNK_IS_USED(next_chunk_header);
        if (!next_chunk_used) {
            size_t next_chunk_size = CHUNK_SIZE(next_chunk_header);
            // update current chunk
            size_t new_size = chunk_size + next_chunk_size;
            uint32_t *new_chunk_header = (uint32_t *)chunk_addr;
            uint32_t *new_chunk_footer = (uint32_t *)(chunk_addr + new_size - WSIZE);
            *new_chunk_header = CHUNK(new_size, CHUNK_PREV_USED(chunk_header), 0);
            *new_chunk_footer = CHUNK(new_size, CHUNK_PREV_USED(chunk_header), 0);
            chunk_size = new_size;
        }
    }

    // merge with previous chunk if free
    if (!CHUNK_PREV_USED(chunk_header)) {
        uintptr_t prev_chunk_footer_addr = chunk_addr - WSIZE;
        uint32_t prev_chunk_footer = *(uint32_t *)prev_chunk_footer_addr;
        size_t prev_chunk_size = CHUNK_SIZE(prev_chunk_footer);
        uintptr_t prev_chunk_addr = chunk_addr - prev_chunk_size;
        // update previous chunk
        size_t new_size = prev_chunk_size + chunk_size;
        uint32_t *new_chunk_header = (uint32_t *)prev_chunk_addr;
        uint32_t *new_chunk_footer = (uint32_t *)(prev_chunk_addr + new_size - WSIZE);
        *new_chunk_header = CHUNK(new_size, CHUNK_PREV_USED(prev_chunk_footer), 0);
        *new_chunk_footer = CHUNK(new_size, CHUNK_PREV_USED(prev_chunk_footer), 0);
    }
}



} // namespace mm