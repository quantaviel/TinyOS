#pragma once

#include <ds/list.h>

namespace sleep {

using waiting_queue = list_node;

void sleep_event(waiting_queue* wq);

void wake_up_one(waiting_queue* wq);

void wake_up_all(waiting_queue* wq);

} // namespace sleep;