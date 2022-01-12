#include "buddy_new.h"
#include "../mem/mem.h"
#include "../drivers/screen.h"

BuddyMemDesc fast_buddy_new;

void init_buddy_new(u32 num_pages, u32 pfn_offset, BuddyMemDesc *buddy_test) {
  u32 rounded_down_pages =
      (num_pages / PAGES_PER_BLOCK(MAX_ORDER)) * PAGES_PER_BLOCK(MAX_ORDER);
  if (rounded_down_pages == 0) panic("Memory chunk is < 4MiB\n");

  buddy_test->all_buddies =
      boot_alloc(rounded_down_pages * sizeof(BuddyBlockNew), TRUE);
  buddy_test->pfn_offset = pfn_offset;
  buddy_test->tot_pages = rounded_down_pages;
  for (u32 i = 0; i < MAX_ORDER + 1; ++i) LIST_INIT(&buddy_test->free_lists[i]);

  for (u32 i = 0; i < rounded_down_pages; ++i) {
    buddy_test->all_buddies[i].free = FALSE;
    LIST_INIT(&buddy_test->all_buddies[i].free_list);
    buddy_test->all_buddies[i].order = 0;
    buddy_test->all_buddies[i].pfn = i;
  }

  u32 i = 0;
  for (; i < rounded_down_pages; i += PAGES_PER_BLOCK(MAX_ORDER)) {
    BuddyBlockNew *top_order_block = &buddy_test->all_buddies[i];
    top_order_block->free = TRUE;
    list_add_tail(&buddy_test->free_lists[MAX_ORDER],
                  &top_order_block->free_list);
    top_order_block->order = MAX_ORDER;
  }
}

void print_buddy_new(BuddyBlockNew *b) {
  kprintf("PFN: %u O: %d F: %d  Phy: 0x%x\n", b->pfn, b->order, b->free,
          calc_buddy_phys(b, &fast_buddy_new));
}

u32 calc_buddy_phys(BuddyBlockNew *b, BuddyMemDesc *mem_desc) {
  return (mem_desc->pfn_offset + b->pfn) * PAGE_SIZE;
}

BuddyBlockNew *buddy_new_from_phys(u32 phys, BuddyMemDesc *mem_desc) {
  u32 pfn = phys / PAGE_SIZE;
  return &mem_desc->all_buddies[pfn - mem_desc->pfn_offset];
}

void print_buddy_status(u32 order, BuddyMemDesc *mem_test) {
  List *l;
  if (list_length(&mem_test->free_lists[order]) > 0)
    kprintf("#Free Buddies of order %d = %d\n", order,
            list_length(&mem_test->free_lists[order]));
  /* list_for_each(l, &mem_test->free_lists[order]) {
    BuddyBlockNew *bb = list_entry(l, BuddyBlockNew, free_list);
    print_buddy_new(bb);
  } */
}

BuddyBlockNew *get_buddy_new(u32 order, BuddyMemDesc *mem_desc) {
  if (order > MAX_ORDER) {
    // for (u32 o = 0; o <= MAX_ORDER; ++o) print_buddy_status(o,
    // &fast_buddy_new);
    kprintf("Order: %d\n", order);
    panic("Invalid order to alloc\n");
    return NULL;
  }
  // bool pi = disable_int();
  List *l;
  BuddyBlockNew *found = NULL;
  list_for_each(l, &mem_desc->free_lists[order]) {
    found = list_entry(l, BuddyBlockNew, free_list);
    found->free = FALSE;
    list_remove(&found->free_list);
    found->order = order;
    // enable_int(pi);
    return found;
  }

  if (order == MAX_ORDER) {
    for (u32 o = 0; o <= MAX_ORDER; ++o) print_buddy_status(o, &fast_buddy_new);
    panic("No more buddy blocks available\n");
  }

  found = get_buddy_new(order + 1, mem_desc);
  found->order = order;
  // Need to break
  u32 b_pfn = found->pfn + (PAGES_PER_BLOCK((order + 1)) / 2);
  BuddyBlockNew *my_buddy = &mem_desc->all_buddies[b_pfn];
  //list_remove(&my_buddy->free_list);
  //list_remove(&found->free_list);

  my_buddy->free = TRUE;
  my_buddy->order = order;
  list_add_head(&mem_desc->free_lists[order], &my_buddy->free_list);
  // enable_int(pi);
  found->free = FALSE;
  return found;
}

u32 calc_my_buddy_new_pfn(BuddyBlockNew *b, u32 order, BuddyMemDesc *mem_desc) {
  u32 buddy_pfn;
  if ((b->pfn / PAGES_PER_BLOCK(order)) % 2 == 0)
    buddy_pfn = b->pfn + PAGES_PER_BLOCK(order);
  else
    buddy_pfn = b->pfn - PAGES_PER_BLOCK(order);
  return buddy_pfn;
}

BuddyBlockNew *calc_free_buddy(BuddyBlockNew *b, BuddyMemDesc *mem_desc) {
  if (b->order >= MAX_ORDER) return NULL;
  u32 buddy_pfn = calc_my_buddy_new_pfn(b, b->order, mem_desc);
  BuddyBlockNew *found_buddy = &mem_desc->all_buddies[buddy_pfn];
  if (found_buddy->free == TRUE && found_buddy->order == b->order)
    return found_buddy;
  else
    return NULL;
}

void free_buddy_new(BuddyBlockNew *b, BuddyMemDesc *mem_desc) {
  if (b->order > MAX_ORDER) {
    panic("Max order exceeded in free!\n");
  }
  if (b->free == TRUE) {
    panic("Trying to free a free buddy\n");
  }
  // bool pi = disable_int();

  // BuddyBlockNew *my_buddy = calc_buddy(b, b->order, mem_desc);
  List *l;
  BuddyBlockNew *list_my_buddy = calc_free_buddy(b, mem_desc);
  if (list_my_buddy != NULL && b->order < MAX_ORDER) {
    // kprintfColor(GREEN, "Buddy free\n");

    // Buddy is free, we need to merge

    /*  kprintf("\n1) Me : ");
     print_buddy_new(b);
     kprintf("2) My buddy: ");
     print_buddy_new(list_my_buddy); */
    // Merge
    b->free = FALSE;
    //list_remove(&b->free_list);
    list_my_buddy->free = FALSE;
    list_remove(&list_my_buddy->free_list);

    BuddyBlockNew *base_bigger_block = b;
    u32 upper_order = b->order + 1;
    if (b->pfn > list_my_buddy->pfn) base_bigger_block = list_my_buddy;
    //list_remove(&base_bigger_block->free_list);
    base_bigger_block->order = upper_order;
    base_bigger_block->free = FALSE;
    /*  list_add_head(&mem_desc->free_lists[b->order + 1],
                   &base_bigger_block->free_list);
  */
    // enable_int(pi);
    free_buddy_new(base_bigger_block, mem_desc);
  } else {
    // kprintfColor(RED, "Buddy not free!\n");
    // bool pi = disable_int();
    b->free = TRUE;
    //list_remove(&b->free_list);
    list_add_head(&mem_desc->free_lists[b->order], &b->free_list);
    // kprintfColor(BLUE, "Freed buddy O: %d\n", b->order);
  }
  // enable_int(pi);
}
