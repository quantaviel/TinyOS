#include <arch/x86/tss.h>
#include <arch/x86/gdt.h>
#include <arch/x86/int.h>

#include <core/process.h>
#include <core/timer.h>

#include <hal/cpu.h>
#include <hal/apic.h>

#include <mm/kalloc.h>
#include <mm/page.h>
#include <mm/pmm.h>
#include <mm/memory.h>

#include <klib/string.h>
#include <klib/fmt.h>

#include <fs/vfs.h>

namespace proc {

#define PROCESS_KERNEL_STACK(p) ((uint32_t)(p) + PAGE_SIZE)

using mm::kvm;

static list_node ready_queue_head;

void append_ready_queue(proc* proc) {
    list_push_back(&ready_queue_head, &proc->ready_node);
}

void remove_ready_queue(proc* proc) {
    list_remove(&proc->ready_node);
}

void popfront_ready_queue() {
    if (!list_empty(&ready_queue_head))
        list_remove(ready_queue_head.next);
}

proc* front_ready_queue() {
    if (list_empty(&ready_queue_head)) return nullptr;
    list_node* f = ready_queue_head.next;
    proc* front_pcb = container_of(f, proc, ready_node);
    return front_pcb;
}

proc kernel_main_pcb;
proc idel_pcb;
proc* current_pcb{nullptr};

constexpr uint32_t user_stack_size = 0x1000;
constexpr uintptr_t user_stack_start = 0xBFFFFFFF;
constexpr uint32_t PID_BITMAP_SIZE = 4096;
static uint8_t pid_bitmap[4096];

extern "C"
void switch_to(proc* prev, proc* curr);

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
    cpu::refresh_all_tlb();
    
    return new_page_dir_phy;
}

void free_proc_page(proc* process) {
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
        "iret;"
    );
}

// create process for first process
proc* create_process(uint32_t entry, const char* name) {
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
    
    proc* new_pcb = (proc*)new_pcb_vir;

    uintptr_t cr3;
    new_pcb->cr3 = copy_page_dir(cr3);
    fmt::println("Copy page directory and page table. New page directory: {x}", new_pcb->cr3);

    new_pcb->pid = get_available_pid();
    new_pcb->state = proc_state::ready;
    new_pcb->quantum = 100;
    strcpy(new_pcb->name, name);
    for (int i = 0; i < MAX_FD; ++i) new_pcb->fd_array[i] = nullptr;
    new_pcb->fd_array[STDIN_FNO] = vfs::dup_file(vfs::default_tty);
    new_pcb->fd_array[STDOUT_FNO] = vfs::dup_file(vfs::default_tty);
    new_pcb->fd_array[STDERR_FNO] = vfs::dup_file(vfs::default_tty);
    new_pcb->sub_proc = new_pcb->next_proc = new_pcb->parent_proc = nullptr;

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
    // set kernel main pcb
    current_pcb = &kernel_main_pcb;
    kernel_main_pcb.pid = 0; pid_mark(0);
    kernel_main_pcb.state = proc_state::system;
    kernel_main_pcb.cr3 = cpu::get_page_dir();

    proc* init_proc = create_process(entry, "init");

    list_init(&ready_queue_head);
    
    fmt::println("create init process.");

    append_ready_queue(init_proc);

    timer::register_event(10, timer::TIMER_PERIODIC, timer::TIMER_TICK, proc_timer_handler, nullptr);
    
    schedule();
}

void schedule() {
    proc* old_proc = current_pcb;

    switch (current_pcb->state) {
        case proc_state::running:
            current_pcb->state = proc_state::ready;
            append_ready_queue(current_pcb);
            break;
        default:
            break;
    }
        
    current_pcb = front_ready_queue();
    current_pcb->remaining = current_pcb->quantum;
    current_pcb->state = proc_state::running;

    popfront_ready_queue();

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

    proc* sub_pcb;
    sub_pcb = (proc* )sub_pcb_addr_vir;
    pid_t sub_pid = get_available_pid();
    frame->eflags = (frame->eflags & ~(0x4000 | 0x100)) | 0x202;
    
    frame->gp.eax = sub_pid;
    memcpy(sub_pcb, current_pcb, PAGE_SIZE);

    auto insert = [](proc* parent, proc* sub) {
        if (parent->sub_proc == nullptr) {
            parent->sub_proc = sub;
            return;
        }
        proc* iter = parent->sub_proc;
        while (iter->next_proc != nullptr) {
            iter = iter->next_proc;
        }
        iter->next_proc = sub;
    };
    
    sub_pcb->pid = sub_pid;
    sub_pcb->parent_proc = current_pcb;
    sub_pcb->sub_proc = sub_pcb->next_proc = nullptr;
    insert(current_pcb, sub_pcb);
    sub_pcb->quantum = 100;
    sub_pcb->state = proc_state::ready;
    strcpy(sub_pcb->name, "sub process");
    for (int i = 0; i < MAX_FD; ++i) {
        if (current_pcb->fd_array[i] != nullptr) {
            sub_pcb->fd_array[i] = vfs::dup_file(current_pcb->fd_array[i]);
        }
    }

    
    uintptr_t sub_cr3;
    sub_pcb->cr3 = copy_page_dir(sub_cr3);
    
    uint32_t offset = (uint32_t)frame - (uint32_t)current_pcb;
    int_frame_t* sub_frame = (int_frame_t*)(sub_pcb_addr_vir + offset);
    sub_frame->gp.eax = 0;
    
    uint32_t* sub_esp = (uint32_t*)sub_frame;

    --sub_esp; *sub_esp = (uint32_t)process_start_wrapper;

    // // push gps
    --sub_esp; *sub_esp = 0;
    --sub_esp; *sub_esp = 0;
    --sub_esp; *sub_esp = 0;
    --sub_esp; *sub_esp = 0;

    sub_pcb->kernel_stack_ptr = (uint32_t)sub_esp;

    // append_ready_queue(sub_pcb);
    append_ready_queue(sub_pcb);
    
    return sub_pcb->pid;
}

proc* get_current_proc() {
    return current_pcb;
}

void sys_exit(int status) {
    for (int i = 0; i < MAX_FD; ++i) {
        if (current_pcb->fd_array[i]) {
            vfs::close(current_pcb->fd_array[i]);
            current_pcb->fd_array[i] = nullptr;
        }
    }
    current_pcb->exit_status = status;
    current_pcb->state = proc_state::zombie;
}

} // namespace proc