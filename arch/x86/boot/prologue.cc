// Some initialization functions before entering virtual memory

#include <arch/x86/multiboot.h>
#include <hal/cpu.h>
#include <hal/vga.h>
#include <mm/page.h>

#include <stdint-gcc.h>

namespace prol {

constexpr static int VGA_WIDTH = 80;
constexpr static int VGA_HEIGHT = 25;

int vga_pos;
uint16_t* vga_buffer;
uint8_t color_attr;

void newline() {
    vga_pos = (vga_pos / VGA_WIDTH) * VGA_WIDTH + VGA_WIDTH;
}
void gofront() {
    vga_pos = (vga_pos / VGA_WIDTH) * VGA_WIDTH;
}

void putchar(char ch) {
    switch (ch) {
        case '\n':
            newline();
            break;
        case '\r':
            gofront();
            break;
        case '\t':
            for (int _ = 0; _ < 4; ++_)
                putchar(' ');
            break;
        default:
            vga_buffer[vga_pos] = (color_attr << 8) | ch;
            vga_pos++;
    }
}

void write(const char* str) {
    while (*str) putchar(*str++);
}

} // namespace prol

extern "C"
void prologue_init() {
    prol::color_attr = (0 << 4) | 7;
    prol::vga_buffer = (uint16_t *)0xB8000;
    prol::vga_pos    = 0;
}

/* save multiboot info from `mb_info` */
extern "C"
void save_multiboot_info(multiboot_info* new_mb_info, multiboot_info* mb_info) {
    prol::write("Save multiboot info ...\n");
    uint32_t mb_size = sizeof(multiboot_info_t);
    uint8_t *nmb = reinterpret_cast<uint8_t*>(new_mb_info);
    uint8_t *mb = reinterpret_cast<uint8_t*>(mb_info);
    for (int i = 0; i < mb_size; ++i) {
        *(nmb + i) = *(mb + i);
    }
    
    uint8_t *current = nmb + mb_size;

    if (mb_info->flags & MULTIBOOT_INFO_MODS) {
        // boot modules enable
        // need to move mods_addr to new place
        uint32_t count = mb_info->mods_count;
        for (int i = 0; i < count * 4 * 4; ++i) {
            *(current + i) = *(uint8_t *)(mb_info->mods_addr + i);
        }
        new_mb_info->mods_addr = reinterpret_cast<multiboot_uint32_t>(current);
        current += count * 4 * 4;
    }
    if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP) {
        // memory map enable
        // need to move mmap_addr to new place
        for (int i = 0; i < mb_info->mmap_length; ++i) {
            *(current + i) = *(uint8_t *)(mb_info->mmap_addr + i);
        }
        new_mb_info->mmap_addr = reinterpret_cast<multiboot_uint32_t>(current);
        current += mb_info->mmap_length;
    }
}

extern "C"
void kernel_page_init(uint32_t* kpg) {
    prol::write("Kernel page initialization ...\n");

    // clear
    for (int i = 0; i < (KPG_SIZE >> 2); ++i) *(kpg + i) = 0;

    uint32_t* kpage_dir   = kpg;
    uint32_t* kpage_table = kpg + 1024;
    
    // construct identity mapping
    set_pd(kpage_dir, 0, PDE(PDE_ATTR, (uintptr_t)&(kpage_table[0])));
    for (int i = 0; i < PROLOGUE_PAGE_COUNT; ++i) {
        set_pt(kpage_table, i, PTE(PTE_ATTR, i << 12));
    }

    // construct kernel mapping
    set_pd(
        kpage_dir,
        PDE_INDEX(KERNEL_START),
        PDE(
            PDE_ATTR,
            (uintptr_t)&kpage_table[(V2P(KERNEL_START) & 0x003FF000) >> 12]
        ));
    for (int i = 0; i < KERNEL_PAGE_COUNT; ++i) {
        set_pt(kpage_table, PTE_INDEX(KERNEL_START + (i << 12)), PTE(PTE_ATTR, V2P(KERNEL_START + (i << 12))));
    }

    // set cycle mapping
    set_pd(kpage_dir, 1023, PDE(PDE_DISABLE_CACHE | PDE_READ_WRITE, (uintptr_t)kpage_dir));

    // test
    // uint32_t addr = 0xc0108000;
    // uint32_t pde = addr >> 22;
    // uint32_t* page_table = (uint32_t *)(kpage_dir[pde] & (0xFFFFF000));
    // uint32_t pte = page_table[(addr & 0x003FF000) >> 12] & 0xFFFFF000;

    // if ((pte | (addr & 0xFFF)) != V2P(addr)) {
    //     prol::write("Error!\n");
    //     while (1);
    // }
}

extern "C"
void prol_vga_init() {
    cpu::pic_disable();
    vga::set_pos(prol::vga_pos);
}