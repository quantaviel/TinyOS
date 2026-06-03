#pragma once

#include <stdint-gcc.h>
#include <arch/x86/int.h>

namespace proc {

enum proc_status {
    PROC_READY,
    PROC_BLOCKED,
    PROC_RUNNING,
    PROC_ZOMBIE
};

using pid_t = int32_t;

// process control block
struct pcb_t {
    uint32_t kernel_stack_ptr; // kernel stack pointer

    uintptr_t cr3; // process page directory address (phy)

    pid_t pid; // process id
    proc_status state; // process status
    uint32_t priority; // process priority
    uint32_t remaining; // process remaining ticks


    // process structure
    pcb_t* parent_proc;
    pcb_t* next_proc;

    char name[20]; // process name
};

using pcb = pcb_t;

auto sys_fork(int_frame_t* frame) -> pid_t;

void init(uint32_t entry);


} // namespace proc