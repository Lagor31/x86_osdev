#include "wait_queue.h"
#include "../mem/mem.h"

WaitQ* create_wait_queue() {
  WaitQ* out = kalloc_page(0);
  LIST_INIT(&out->threads_waiting);

  return out;
}

WaitQ* create_wait_queue_nosleep() {
  WaitQ* out = kalloc_nosleep(0);
  LIST_INIT(&out->threads_waiting);

  return out;
}
void destroy_wait_queue(WaitQ* wq) {
  list_remove(&wq->threads_waiting);
  kfree_page(wq);
}