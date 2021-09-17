#ifndef WAITQ_H
#define WAITQ_H

#include "../lib/list.h"

typedef struct wait_q_t {
  List threads_waiting;
} WaitQ;

WaitQ* create_wait_queue();
WaitQ* create_wait_queue_nosleep();
void destroy_wait_queue(WaitQ* wq);
#endif