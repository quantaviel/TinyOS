#pragma once

#include <stdint-gcc.h>
#include <ds/list.h>
#include <arch/x86/int.h>
#include <fs/vfs.h>

#define MAX_FD 16

namespace proc {

enum class proc_state {
    ready,
    blocked,
    running,
    zombie,
    system,
};

using pid_t = int32_t;

// process control block
struct proc {
    uint32_t kernel_stack_ptr; // kernel stack pointer

    uintptr_t cr3; // process page directory address (phy)

    pid_t pid; // process id
    proc_state state; // process state
    uint32_t quantum; // process quantum
    uint32_t remaining; // process remaining quantums
    int32_t exit_status;

    // process structure
    proc* parent_proc;
    proc* sub_proc;
    proc* next_proc;

    list_node_t ready_node;
    list_node_t waiting_node;

    char name[20]; // process name
    vfs::file* fd_array[MAX_FD];
};

extern proc* current_pcb;

void schedule();
auto sys_fork(int_frame_t* frame) -> pid_t;
void sys_exit(int status);

void init(uint32_t entry);

void append_ready_queue(proc* p);
void remove_ready_queue(proc* p);


} // namespace proc