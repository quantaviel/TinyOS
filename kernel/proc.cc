#include <arch/x86/tss.h>
#include <arch/x86/gdt.h>
#include <arch/x86/int.h>

#include <process.h>

#include <timer.h>

#include <hal/cpu.h>
#include <hal/apic.h>

#include <mm/kalloc.h>
#include <mm/page.h>
#include <mm/pmm.h>
#include <mm/memory.h>

#include <klib/string.h>
#include <klib/fmt.h>

extern tss_t global_tss;
extern uint32_t __user_code_start;
extern uint32_t __user_code_end;

#define USER_CODE_START     sys_val(__user_code_start)
#define USER_CODE_END       sys_val(__user_code_end)

#define PROCESS_KERNEL_STACK(pcb_p) ((uint32_t)(pcb_p) + PAGE_SIZE)

namespace proc {

using mm::kvm;

static pcb* ready_queue_head = nullptr;
static pcb* ready_queue_tail = nullptr;

void append_ready_queue(pcb* proc) {
    if (ready_queue_head == ready_queue_tail && ready_queue_head == nullptr) {
        ready_queue_head = ready_queue_tail = proc;
        return;
    }
    ready_queue_tail->next_proc = proc;
    ready_queue_tail = proc;
}

void pop_ready_queue() {
    if (ready_queue_head == nullptr) return;
    ready_queue_head = ready_queue_head->next_proc;
}

static pcb kernel_main_pcb;
static pcb* current_pcb = nullptr;

constexpr uint32_t user_stack_size = 0x1000;
constexpr uintptr_t user_stack_start = 0xBFFFFFFF;
constexpr uint32_t PID_BITMAP_SIZE = 4096;
static uint8_t pid_bitmap[4096];

extern "C"
void switch_to(pcb* prev, pcb* curr);

void pid_mark(pid_t pid) {
    pid_bitmap[pid >> 3] |= (1 << (pid & 7));
}

void pid_unmark(pid_t pid) {
    pid_bitmap[pid >> 3] &= ~(1 << (pid & 7));
}

pid_t get_available_pid() {
    for (int i = 0; i < PID_BITMAP_SIZE; ++i) {
        if (pid_bitmap[i] == 0xFF) continue;
        int bit = __builtin_ffs(~pid_bitmap[i]) - 1;
        pid_mark(i << 3 | bit);
        return i << 3 | bit;
    }
    return -1;
}

void copy_page_table(uint32_t* dst_page_table, uint32_t* src_page_table) {
    for (int i = 0; i < 1024; ++i) {
        if (src_page_table[i] & PTE_PRESENT) {
            uintptr_t paddr = src_page_table[i] & 0xFFFFF000;
            mm::pmm::inc_refcount(paddr);
            src_page_table[i] &= ~PTE_READ_WRITE;
            dst_page_table[i] = src_page_table[i];
        }
    }
}

auto copy_page_dir(uint32_t& cr3_vir) -> uintptr_t {
    uint32_t* page_dir = (uint32_t *)PD_VADDR;
    uintptr_t new_page_dir_phy = mm::pmm::alloc_page();
    uintptr_t new_page_dir_vir = mm::vmm::alloc_bit_page();

    if (!new_page_dir_phy) {
        fmt::println("Error: Unable to alloc memory for page directory!");
        return 0;
    }

    // create temporal mapping
    mm::pt::map(new_page_dir_vir, new_page_dir_phy, PDE_ATTR, PTE_ATTR);

    uint32_t* new_page_dir = (uint32_t*)new_page_dir_vir;
    memset(new_page_dir, 0, sizeof(int) * 1024);

    // kernel page: direct copy
    for (int i = 0x300; i < 0x3FF; ++i) {
        new_page_dir[i] = page_dir[i];
    }
    // user page: copy on write
    for (int i = 0; i < 0x300; ++i) {
        if (page_dir[i] & 1) { // page director exist
            uint32_t* page_table = (uint32_t*)(PT_BASE_VADDR + (i << PAGE_SHIFT));
            uintptr_t new_page_table = mm::pmm::alloc_page();
            new_page_dir[i] = new_page_table | (page_dir[i] & 0xFFF);

            uintptr_t new_page_table_vir = mm::vmm::alloc_bit_page();
            mm::pt::map(new_page_table_vir, new_page_table, PDE_ATTR, PTE_ATTR);
            
            copy_page_table((uint32_t*)new_page_table_vir, page_table);
            
            mm::pt::unmap(new_page_table_vir);
            mm::vmm::free_bit_page(new_page_table_vir);
        }
    }

    // cycle mapping
    new_page_dir[1023] = PDE(PDE_DISABLE_CACHE | PDE_READ_WRITE, new_page_dir_phy);
    cr3_vir = new_page_dir_vir;
    
    // refresh all tlb
    uintptr_t current_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(current_cr3));
    asm volatile("mov %0, %%cr3" :: "r"(current_cr3) : "memory");
    
    return new_page_dir_phy;
}

void free_proc_page(pcb* process) {
    /// TODO: free page memory with die process
}

extern "C"
__attribute__((naked)) void process_start_wrapper() {
    asm volatile (
        "popl %eax;"  // 对应 gp_regs.eax (最低地址/栈顶)
        "popl %ebx;"  // 对应 gp_regs.ebx
        "popl %ecx;"  // 对应 gp_regs.ecx
        "popl %edx;"  // 对应 gp_regs.edx
        "popl %edi;"  // 对应 gp_regs.edi
        "popl %ebp;"  // 对应 gp_regs.ebp
        "popl %esi;"  // 对应 gp_regs.esi
        "addl $12, %esp;"
    );
    asm volatile (
        "iret;"
    );
}

// create process for first process
pcb* create_process(uint32_t entry, const char* name) {
    uintptr_t new_pcb_phy = mm::pmm::alloc_page();
    uintptr_t new_pcb_vir = mm::vmm::alloc_bit_page();
    uintptr_t new_ker_stack_phy = PROCESS_KERNEL_STACK(new_pcb_phy);
    uintptr_t new_ker_stack_vir = PROCESS_KERNEL_STACK(new_pcb_vir);
    
    if (!new_pcb_phy || !new_pcb_vir) {
        fmt::println("Error: Unable to alloc memory for new process block!");
        if (!new_pcb_phy) mm::pmm::free_page(new_pcb_phy);
        if (!new_pcb_vir) mm::vmm::free_bit_page(new_pcb_vir);
        return 0;
    }

    mm::pt::map(new_pcb_vir, new_pcb_phy, PDE_ATTR, PTE_ATTR);
    
    pcb* new_pcb = (pcb*)new_pcb_vir;

    uintptr_t cr3;
    
    new_pcb->cr3 = copy_page_dir(cr3);
    fmt::println("Copy page directory and page table. New page directory: {x}", new_pcb->cr3);

    new_pcb->pid = 1;
    pid_mark(1);
    new_pcb->state = PROC_READY;
    new_pcb->priority = 100;
    strcpy(new_pcb->name, name);

    fmt::println("Create user stack.");
    uintptr_t user_stack = mm::pmm::alloc_page();
    mm::pt::map_to_page_dir(cr3, user_stack_start - user_stack_size + 1, user_stack, PDE_PRESENT | PDE_READ_WRITE | PDE_USER_AVAL, PTE_PRESENT | PTE_READ_WRITE | PTE_USER_AVAL);

    // Create stack for iret
    fmt::println("Create stack for iret");
    uint32_t* v_stack = (uint32_t*)new_ker_stack_vir;

#define PUSH(x) do { (--v_stack); *v_stack = (x); } while (0);
    PUSH(SEGMENT_R3_DATA); // ss: user data segment selector
    PUSH(user_stack_start); // esp: user stack
    PUSH(0x202); // eflags: user eflags
    PUSH(SEGMENT_R3_CODE); // cs: user code segment selector
    PUSH(entry); // eip: user entry-0,

    // interrupt vector and errorcode
    PUSH(0);
    PUSH(0);

    for (int i = 0; i < 8; ++i) {
        PUSH(0); // universe register
    }

    PUSH((uint32_t)process_start_wrapper);

    PUSH(0); // ebp
    PUSH(0); // ebx
    PUSH(0); // edi
    PUSH(0); // esi
#undef PUSH

    new_pcb->kernel_stack_ptr = (uint32_t)v_stack;

    // Unmap temporal map for new process cr3
    mm::pt::unmap(cr3);
    mm::vmm::free_bit_page(cr3);

    return new_pcb;
}

void schedule();
void proc_timer_handler(void*);

void init(uint32_t entry) {
    pcb* init_proc = create_process(entry, "init");
    
    fmt::println("create init process.");

    // set kernel main pcb
    current_pcb = &kernel_main_pcb;
    kernel_main_pcb.pid = 0;
    pid_mark(0);
    uintptr_t current_cr3 = 0;
    asm volatile("movl %%cr3, %0": "=r"(current_cr3));
    kernel_main_pcb.cr3 = current_cr3;

    ready_queue_head = init_proc;

    timer::register_event(10, timer::TIMER_PERIODIC, timer::TIMER_TICK, proc_timer_handler, nullptr);
    
    schedule();
    return;
}

void schedule() {
    pcb* old_proc = current_pcb;
    if (old_proc != &kernel_main_pcb) append_ready_queue(old_proc);
    if (ready_queue_head == nullptr) {
        current_pcb->remaining = current_pcb->priority;
        return;
    }

    current_pcb->state = PROC_READY;
    
    current_pcb = ready_queue_head;
    current_pcb->remaining = current_pcb->priority;
    current_pcb->state = PROC_RUNNING;

    pop_ready_queue();

    global_tss.esp0 = PROCESS_KERNEL_STACK(current_pcb);
    switch_to(old_proc, current_pcb);
}

void proc_timer_handler(void *) {
    if (current_pcb == &kernel_main_pcb) return;
    current_pcb->remaining--;
    if (current_pcb->remaining == 0) {
        schedule();
    }
}

pid_t sys_fork(int_frame_t* frame) {
    uintptr_t sub_pcb_addr_phy = mm::pmm::alloc_page();
    uintptr_t sub_pcb_addr_vir = mm::vmm::alloc_bit_page();

    uintptr_t sub_ker_stack_phy = PROCESS_KERNEL_STACK(sub_pcb_addr_phy);
    uintptr_t sub_ker_stack_vir = PROCESS_KERNEL_STACK(sub_pcb_addr_vir);
    
    if (!sub_pcb_addr_phy || !sub_pcb_addr_vir) {
        fmt::println("Error: Unable to alloc memory for new process block!");
        if (!sub_pcb_addr_phy) mm::pmm::free_page(sub_pcb_addr_phy);
        if (!sub_pcb_addr_vir) mm::vmm::free_bit_page(sub_pcb_addr_vir);
        return 0;
    }

    mm::pt::map(sub_pcb_addr_vir, sub_pcb_addr_phy, PDE_ATTR, PTE_ATTR);

    pcb* sub_pcb;
    sub_pcb = (pcb* )sub_pcb_addr_vir;
    pid_t sub_pid = get_available_pid();
    frame->eflags = (frame->eflags & ~(0x4000 | 0x100)) | 0x202;
    
    frame->gp.eax = sub_pid;
    memcpy(sub_pcb, current_pcb, PAGE_SIZE);
    
    sub_pcb->pid = sub_pid;
    sub_pcb->parent_proc = current_pcb;
    sub_pcb->priority = 100;
    sub_pcb->state = PROC_READY;
    strcpy(sub_pcb->name, "sub process");

    
    uintptr_t sub_cr3;
    sub_pcb->cr3 = copy_page_dir(sub_cr3);
    
    uint32_t offset = (uint32_t)frame - (uint32_t)current_pcb;
    int_frame_t* sub_frame = (int_frame_t*)(sub_pcb_addr_vir + offset);
    sub_frame->gp.eax = 0;
    
    uint32_t* sub_esp = (uint32_t*)sub_frame;

    --sub_esp; *sub_esp = (uint32_t)process_start_wrapper;

    // push gps
    --sub_esp; *sub_esp = sub_frame->gp.esi;
    --sub_esp; *sub_esp = sub_frame->gp.edi;
    --sub_esp; *sub_esp = sub_frame->gp.ebx;
    --sub_esp; *sub_esp = sub_frame->gp.ebp;

    sub_pcb->kernel_stack_ptr = (uint32_t)sub_esp;

    append_ready_queue(sub_pcb);
    
    return sub_pcb->pid;
}

} // namespace proc