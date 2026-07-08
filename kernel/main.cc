#include <arch/x86/multiboot.h>
#include <arch/x86/iroute.h>
#include <arch/x86/tss.h>
#include <klib/fmt.h>
#include <klib/string.h>
#include <hal/acpi/acpi.h>
#include <hal/apic.h>
#include <hal/cpu.h>
#include <hal/ioapic.h>
#include <hal/keyboard.h>
#include <mm/page.h>
#include <mm/pmm.h>
#include <mm/kalloc.h>
#include <core/process.h>
#include <core/timer.h>
#include <hal/vga.h>
#include <core/syscall.h>
#include <driver/ata.h>
#include <fs/fs.h>

/**
 * functions called in assembly should use
 * `extern "C"`, so that function name will not
 * be rewrite.
*/


void memory_init(multiboot_info_t* mbi);
extern "C" void kernel_main();

extern "C"
void kernel_init(multiboot_info* mb_info) {
    fmt::println("multiboot info addr: {x}", mb_info);

    if (mb_info->flags & MULTIBOOT_INFO_MEMORY) {
        fmt::println("Memory Info: {x}, {x}", mb_info->mem_lower, mb_info->mem_upper);
    } else {
        fmt::println("Error: No memory info provided by multiboot.");
    }
    if (!(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        fmt::println("Error: No memory map provided by multiboot.");
        return;
    }

    fmt::println("Total memory: {} KB", mb_info->mem_upper + 1024);
    for (uint8_t *mmap_ptr = (uint8_t*)mb_info->mmap_addr; mmap_ptr < (uint8_t*)mb_info->mmap_addr + mb_info->mmap_length; ) {
        multiboot_memory_map_t *entry = (multiboot_memory_map_t *) mmap_ptr;
        fmt::println("start: {x}, length: {}, type: {}, size: {}", (uint64_t)entry->addr, (uint64_t)entry->len, (uint32_t)entry->type, (uint32_t)entry->size);
        mmap_ptr += entry->size + sizeof(entry->size);
    }

    memory_init(mb_info);

    // heap init
    fmt::println("kernel heap start at {x}, size = {}", KERNEL_HEAP_START, mm::kheap_size);
    mm::kvm::init(KERNEL_HEAP_START, KERNEL_HEAP_START + mm::kheap_size);

    // Memory map for local apic and io apic
    auto acpi_ctx = acpi_get_context();
    mm::pt::map(LAPIC_BASE_VADDR, apic::get_base_addr(), PDE_ATTR, PTE_ATTR);
    mm::pt::map(IOAPIC_BASE_VADDR, acpi_ctx->madt->ioapic->addr, PDE_ATTR, PTE_ATTR);

    ioapic::init();
    apic::init();

    timer::init();
    keyboard::init();
}

// mark and map necessary memory
void memory_init(multiboot_info_t* mbi) {
    mm::pmm::init(mbi);
    mm::vmm::bitmap_init(KERNEL_BITMAP_START, KERNEL_BITMAP_END);
    // unmark available memory
    auto mmap_p = (uint8_t *)mbi->mmap_addr;
    for (; (uintptr_t) mmap_p < mbi->mmap_addr + mbi->mmap_length; ) {
        auto mmap = (multiboot_memory_map_t *)mmap_p;
        /*
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
        */
        switch (mmap->type) {
        case MULTIBOOT_MEMORY_AVAILABLE:
            mm::pmm::unmark_pages(mmap->addr, mmap->len);
            break;
        case MULTIBOOT_MEMORY_RESERVED:
            // fmt::println("addr: {x}, mmap->len: {}", (uint32_t)mmap->addr, (uint32_t)mmap->len);
            mm::pt::map_addrs(mmap->addr, mmap->addr, mmap->len, PDE_ATTR, PTE_ATTR);
            break;
        default:
            break;
        }
        mmap_p = mmap_p + mmap->size + sizeof(mmap->size);
    }
    
    // mark high half kernel
    // kernel is linked at KERNEL_START (virtual), already identity mapped in prologue
    // but we need to mark the corresponding physical pages as used
    mm::pmm::mark_pages(V2P(KERNEL_START), KERNEL_END - KERNEL_START);

    // construct stack - allocate and map kernel stack
    uint32_t stack_size = KERNEL_STACK_SIZE;
    mm::vaddr_t stack_vaddr = KERNEL_STACK_START - stack_size + 1;
    mm::paddr_t stack_paddr = mm::vmm::alloc_pages(stack_vaddr, stack_size);

    // mark vga
    mm::pmm::mark_pages(vga::VGA_PHY_ADDR, vga::VGA_BUFFER_SIZE);
    mm::pt::map_addrs(vga::VGA_VIR_ADDR, vga::VGA_PHY_ADDR, vga::VGA_BUFFER_SIZE, PDE_ATTR, PTE_ATTR);
    vga::set_addr(vga::VGA_VIR_ADDR);
}

void main();

int global_var = 0;
extern "C"
void kernel_main() {
    // global variable test
    fmt::println("virtual memory address of global variable: {x}", &global_var);
    int local_var = 0;
    fmt::println("virtual memory address of local variable: {x}", &local_var);
    // test small kalloc
    int *ptr = (int *)mm::kvm::kalloc(sizeof(int));
    *ptr = 123456;
    fmt::println("Allocated int at {x}, value = {}", ptr, *ptr);
    mm::kvm::kfree(ptr);

    // test large kalloc
    size_t large_size = 5000;
    char *large_ptr = (char *)mm::kvm::kalloc(large_size);
    for (size_t i = 0; i < large_size; ++i) {
        large_ptr[i] = (char)(i % 256);
    }
    fmt::println("Allocated large chunk at {x}, size = {}", large_ptr, large_size);
    mm::kvm::kfree(large_ptr);
    // auto event = timer::register_event(
    //     1, timer::TIMER_PERIODIC, timer::TIMER_TICK,
    //     [](void* arg) { fmt::println("Tick!"); }, nullptr
    // ); 

    // int count = 0;
    // auto sec_event = timer::register_event(
    //     2048, timer::TIMER_PERIODIC, timer::TIMER_SEC,
    //     [](void* arg) { fmt::println("Second! {}\n", *(int*)arg); ++*(int*)arg; }, &count
    // );
    init_tss();
    syscall_init();
    fs::init();
    proc::init((uint32_t)main);
    
    // unreachable
    while (1) {
        // Idle loop
    }
}

void kernel_idel_function() {
    while (1) {
        cpu::halt();
    }
}


void main() {
    write(STDOUT_FNO, "stdout test\n", 12);
    int pid = fork();
    if (pid == 0) {
        write(STDOUT_FNO, "Sub Process!\n", 13);
    } else {
        write(STDOUT_FNO, "Parent Process!\n", 16);
    }
    while (1);
}