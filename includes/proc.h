#pragma once

#include <stdint-gcc.h>

namespace proc {

enum status {
    READY,
    BLOCKED,
    RUNNING,
    DIE
};

using pid_t = int32_t;

// process control block
struct pcb {
    uint32_t kesp; // kernel stack pointer

    uintptr_t cr3; // process page directory address (phy)

    pid_t pid; // process id
    status proc_stat; // process status
    uint32_t priority; // process priority
    uint32_t remaining; // process remaining ticks

    
    // process structure
    pcb* parent;
    pcb* next;

    char name[20]; // process name
};

auto sys_fork(int_frame* frame) -> pid_t;

void init(uint32_t entry);


} // namespace proc