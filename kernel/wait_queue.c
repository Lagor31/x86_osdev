#include "wait_queue.h"
#include "../mem/mem.h"

WaitQ* create_wait_queue() {
  WaitQ* out = normal_page_alloc(0);
  LIST_INIT(&out->threads_waiting);

  return out;
}

void destroy_wait_queue(WaitQ* wq) { kfree_normal(wq); }