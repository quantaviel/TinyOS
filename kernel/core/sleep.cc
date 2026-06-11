#include <core/sleep.h>
#include <core/process.h>

namespace sleep {

void sleep_event(waiting_queue* wq) {
    proc::proc* current = proc::get_current_proc();

    if (current->state == proc::PROC_BLOCKED) return;
    current->state = proc::PROC_BLOCKED;

    list_push_back(wq, &current->waiting_node);

    proc::schedule();
}

void wake_up_one(waiting_queue* wq) {
    if (list_empty(wq)) return;
    list_node* node = wq->next;
    list_remove(node);

    proc::proc* front = container_of(node, proc::proc, waiting_node);
    front->state = proc::PROC_READY;
    proc::append_ready_queue(front);
}

void wake_up_all(waiting_queue* wq) {
    while (!list_empty(wq)) {
        wake_up_one(wq);
    }
}

} // namespace sleep;