#include "buddy.h"
#include "../kernel/mem.h"
#include "../utils/list.h"
#include "../cpu/types.h"

Buddy buddy[BUDDY_ORDER];

void buddy_init() {
  int i = 0;
  List *head = &frames[0].list;
  LIST_INIT(head);

  for (i = 0; i < BUDDY_ORDER; ++i) {
    buddy[i].bitmap = boot_alloc(boot_mmap.total_pages / (1 << i), 1);
    if (i == BUDDY_ORDER - 1) {
      buddy[i].free_buddy_list = &frames[0];
      frames[0].buddy_info = BUDDY_ORDER - 1;
      List *head = &frames[0].list;
      LIST_INIT(head);
      int k = 1 << i;
      while (k < boot_mmap.total_pages) {
        list_add(head, &frames[k].list);
        frames[k].buddy_info = BUDDY_ORDER - 1;
        k += (1 << i);
      }
    } else {
      buddy[i].free_buddy_list = NULL;
    }
  }
}