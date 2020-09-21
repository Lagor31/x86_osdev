#include "page.h"
#include "buddy.h"

#include "../cpu/types.h"
#include "../drivers/screen.h"
#include "../kernel/multiboot.h"

#include "../kernel/mem.h"
#include "../utils/list.h"

Buddy buddy[MAX_ORDER + 1];

void buddy_init() {
  // Allocating array of pages
  frames = boot_alloc(sizeof(Page) * boot_mmap.total_pages, 1);
  int curr_order = 0;
  List *head = &(frames[0].list);
  LIST_INIT(head);
  int order_count = 0;

  for (curr_order = MAX_ORDER; curr_order >= 0; --curr_order) {
    kprintf("Order %d", curr_order);
    if (curr_order == MAX_ORDER) {
      buddy[MAX_ORDER].free_buddy_list = &frames[0];
      frames[0].buddy_info = MAX_ORDER;
      List *free_list = &buddy[MAX_ORDER].free_buddy_list->list;
      LIST_INIT(free_list);
      uint32_t k = 0;
      while (k <= (boot_mmap.total_pages - (1 << MAX_ORDER))) {
        order_count++;
        list_add(free_list, &frames[k].list);
        frames[k].buddy_info = MAX_ORDER;
        k += (1 << MAX_ORDER);
      }
      kprintf(" - OOC= %d", order_count);
      order_count &= 0xFFFFFFFE;
    } else {
      order_count *= 2;
      buddy[curr_order].free_buddy_list = NULL;
    }
    int calculated_oc = boot_mmap.total_pages / (1 << curr_order);
    kprintf(" ROC=%d CCO=%d)\n", order_count, calculated_oc);
    buddy[curr_order].bitmap = boot_alloc(order_count, 1);
  }
  kprintf("Wasted %d pages\n", boot_mmap.total_pages - order_count);
}