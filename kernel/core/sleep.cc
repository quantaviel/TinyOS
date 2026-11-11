#include <core/sleep.h>
#include <core/process.h>

namespace sleep {

void sleep_event(waiting_queue* wq) {

    if (proc::current_pcb->state == proc::proc_state::blocked) return;
    proc::current_pcb->state = proc::proc_state::blocked;

    list_push_back(wq, &proc::current_pcb->waiting_node);

    proc::schedule();
}

void wake_up_one(waiting_queue* wq) {
    if (list_empty(wq)) return;
    list_node* node = wq->next;
    list_remove(node);

    proc::proc* front = container_of(node, proc::proc, waiting_node);
    front->state = proc::proc_state::ready;
    proc::append_ready_queue(front);
}

void wake_up_all(waiting_queue* wq) {
    while (!list_empty(wq)) {
        wake_up_one(wq);
    }
}

} // namespace sleep;