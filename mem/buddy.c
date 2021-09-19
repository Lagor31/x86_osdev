#include "page.h"

#include "../cpu/types.h"
#include "../lib/utils.h"

#include "buddy.h"
#include "../kernel/kernel.h"

#include "../drivers/screen.h"
#include "../boot/multiboot.h"

#include "../mem/mem.h"
#include "../lib/list.h"

Buddy buddy[MAX_ORDER + 1];
Buddy normal_buddy[MAX_ORDER + 1];
Buddy fast_buddy[MAX_ORDER + 1];

u32 phys_normal_offset = 0;
u32 phys_fast_offset = 0;

BuddyBlock *buddies;
BuddyBlock *normal_buddies;
BuddyBlock *fast_buddies;

u32 total_used_memory = 0;
u32 total_kused_memory = 0;
u32 total_nused_memory = 0;
u32 total_fused_memory = 0;

u32 get_pfn_from_page(Page *p, u8 alloc_type) {
  // kprintf("Calculating pfn for addr 0x%x - 0x%x\n", p,
  //        kernel_alloc == 1 ? kernel_pages : normal_pages);
  if (alloc_type == KERNEL_ALLOC)
    return ((u32)p - (u32)kernel_pages) / sizeof(Page);
  else if (alloc_type == FAST_ALLOC)
    return ((u32)p - (u32)fast_pages) / sizeof(Page);
  else
    return ((u32)p - (u32)normal_pages) / sizeof(Page);
}

void *phys_from_pfn(u32 pfn, u8 alloc_type) {
  if (alloc_type == KERNEL_ALLOC)
    return (void *)(pfn * PAGE_SIZE);
  else if (alloc_type == FAST_ALLOC)
    return (void *)(pfn * PAGE_SIZE + phys_fast_offset);
  else
    return (void *)(pfn * PAGE_SIZE + phys_normal_offset);
}

Page *get_page_from_address(void *ptr, u8 alloc_type) {
  if (alloc_type == KERNEL_ALLOC)
    return (Page *)((u32)kernel_pages +
                    (PA((u32)ptr) / PAGE_SIZE) * sizeof(Page));
  else if (alloc_type == FAST_ALLOC) {
    return (Page *)((u32)fast_pages +
                    ((u32)(PA((u32)ptr) - phys_fast_offset) / PAGE_SIZE) * sizeof(Page));
  } else
    return (Page *)((u32)normal_pages +
                    ((u32)(ptr - phys_normal_offset) / PAGE_SIZE) *
                        sizeof(Page));
}

void *get_page_phys_address(Page *p, u8 alloc_type) {
  return phys_from_pfn(get_pfn_from_page(p, alloc_type), alloc_type);
}

void *get_free_address(BuddyBlock *b, u8 alloc_type) {
  return (BuddyBlock *)phys_from_pfn(get_pfn_from_page(b->head, alloc_type),
                                     alloc_type);
}

void printBuddy(BuddyBlock *b, u8 kernel_alloc) {
  u32 physAddr = 0;

  physAddr = (u32)get_page_phys_address(b->head, kernel_alloc);
  kprintf("[%x]->%x S=%d PagePtr=%x PhysAddr=%x\n", b,
          get_free_address(b, kernel_alloc), b->order, b->head, physAddr);
}

void buddy_init(Page **input_pages, BuddyBlock **buddies_ext, Buddy *buddy_ext,
                u32 number_of_pages) {
  // Allocating array of pages
  Page *tPage = boot_alloc(sizeof(Page) * number_of_pages, 1);
  *input_pages = tPage;

  kprintf("Pages Addr: 0x%x, Pages Val: 0x%x\nPages_base_addr: 0x%x\n",
          &input_pages, input_pages, *input_pages);
  int curr_order = MAX_ORDER;
  u32 number_of_blocks = number_of_pages / PAGES_PER_BLOCK(MAX_ORDER);
  // blocks_number &= 0xFFFFFFFE;
  /*   if (!kernel_alloc) hlt();
   */
  BuddyBlock *bb =
      (BuddyBlock *)boot_alloc(sizeof(BuddyBlock) * number_of_pages, 1);
  // buddies = bb;
  *buddies_ext = bb;

  u32 i = 0;
  for (i = 0; i < number_of_pages; ++i) {
    bb[i].head = (*input_pages + i);
  }

  for (curr_order = MAX_ORDER; curr_order >= 0; --curr_order) {
    LIST_INIT(&buddy_ext[curr_order].free_list);

    if (curr_order == MAX_ORDER) {
      kprintf("Order %d Total Blocks %d\n", curr_order, number_of_blocks);
      for (i = 0; i < number_of_blocks; ++i) {
        int curr_buddy_pos = i * PAGES_PER_BLOCK(MAX_ORDER);
        bb[curr_buddy_pos].order = MAX_ORDER;
        list_add_tail(&buddy_ext[curr_order].free_list,
                      &bb[curr_buddy_pos].item);
        // total_free_memory += PAGES_PER_BLOCK(MAX_ORDER) * PAGE_SIZE;
        /*  if (kernel_alloc)
           total_kfree_memory += PAGES_PER_BLOCK(MAX_ORDER) * PAGE_SIZE;
         else
           total_nfree_memory += PAGES_PER_BLOCK(MAX_ORDER) * PAGE_SIZE; */
        /*  if (i <= 3) {
           kprintf("%d : {Addr : %x, Page: %d, O: %d, Buddy: %x}\n",
                   curr_buddy_pos, &bb[curr_buddy_pos],
                   get_pfn_from_page(bb[curr_buddy_pos].head),
                   bb[curr_buddy_pos].order, find_buddy(&bb[curr_buddy_pos]));
         } */
      }
      kprintf("\n");
    } else {
      number_of_blocks *= 2;
    }

    // TODO: Allocate bits, not bytes
    buddy_ext[curr_order].bitmap =
        boot_alloc(sizeof(byte) * number_of_blocks, 1);
    buddy_ext[curr_order].bitmap_length = number_of_blocks;

    if (curr_order == MAX_ORDER)
      memset(buddy_ext[MAX_ORDER].bitmap, FREE, number_of_blocks);
    else
      memset(buddy_ext[curr_order].bitmap, USED, number_of_blocks);
  }
}

u32 get_buddy_pos(BuddyBlock *b, u8 alloc_type) {
  if (alloc_type == KERNEL_ALLOC)
    return ((u32)b - (u32)buddies) / sizeof(BuddyBlock);
  else if (alloc_type == FAST_ALLOC)
    return ((u32)b - (u32)fast_buddies) / sizeof(BuddyBlock);
  else
    return ((u32)b - (u32)normal_buddies) / sizeof(BuddyBlock);
}

bool is_buddy_free_at_order(BuddyBlock *b, u8 order, bool alloc_type) {
  int block_pos =
      get_pfn_from_page(b->head, alloc_type) / PAGES_PER_BLOCK(order);
  if (alloc_type == KERNEL_ALLOC) {
    return buddy[order].bitmap[block_pos];
  } else if (alloc_type == FAST_ALLOC) {
    return fast_buddy[order].bitmap[block_pos];
  } else {
    return normal_buddy[order].bitmap[block_pos];
  }
}

bool is_buddy_block_free(BuddyBlock *b, bool alloc_type) {
  int block_pos =
      get_pfn_from_page(b->head, alloc_type) / PAGES_PER_BLOCK(b->order);
  if (alloc_type == KERNEL_ALLOC) {
    // u32 pos = get_buddy_pos(b, kernel_alloc);
    bool isFree = buddy[b->order].bitmap[block_pos];
    /*  kprintf("Kernel Buddy at pos %d, o:%d, free=%d\n", block_pos, b->order,
             isFree); */
    return isFree;
  } else if (alloc_type == FAST_ALLOC) {
    return fast_buddy[b->order].bitmap[block_pos];
  } else {
    return normal_buddy[b->order].bitmap[block_pos];
  }
}

void free_buddy_block(BuddyBlock *b, u8 alloc_type) {
  for (int i = b->order; i <= MAX_ORDER; ++i) {
    if (is_buddy_free_at_order(b, i, alloc_type)) {
      setBackgroundColor(RED);
      setTextColor(WHITE);
      kprintf("Buddy already freed!!\n");
      printBuddy(b, alloc_type);
      resetScreenColors();
      return;
    }
  }

  BuddyBlock *my_buddy = find_buddy(b, alloc_type);
  u8 need_to_merge =
      is_buddy_block_free(my_buddy, alloc_type) && (b->order != MAX_ORDER);

  if (need_to_merge) {
    // I set myself and my buddy as used
    set_block_usage(b, b->order, USED, alloc_type);
    set_block_usage(my_buddy, my_buddy->order, USED, alloc_type);
    list_remove(&b->item);
    list_remove(&my_buddy->item);
    if (get_buddy_pos(my_buddy, alloc_type) < get_buddy_pos(b, alloc_type)) {
      b = my_buddy;
    }

    u8 higher_order = b->order + 1;
    b->order = higher_order;
    if (alloc_type == KERNEL_ALLOC)
      list_add_tail(&buddy[higher_order].free_list, &b->item);
    else if (alloc_type == FAST_ALLOC)
      list_add_tail(&fast_buddy[higher_order].free_list, &b->item);
    else
      list_add_tail(&normal_buddy[higher_order].free_list, &b->item);
    set_block_usage(b, b->order, FREE, alloc_type);
  } else {
    // Adding block to its free list
    if (alloc_type == KERNEL_ALLOC)
      list_add_tail(&buddy[b->order].free_list, &b->item);
    else if (alloc_type == FAST_ALLOC)
      list_add_tail(&fast_buddy[b->order].free_list, &b->item);
    else
      list_add_tail(&normal_buddy[b->order].free_list, &b->item);
    // Mark it free
    set_block_usage(b, b->order, FREE, alloc_type);
  }
  /*  total_free_memory += PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
   if (kernel_alloc)
     total_kfree_memory += PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
   else */
  // total_nused_memory += PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
}

BuddyBlock *get_buddy_block(int order, u8 alloc_type) {
  BuddyBlock *found = search_free_block(order, alloc_type);
  if (found != NULL) {
    // Found a block in the free list, i need to mark it used and return it
    set_block_usage(found, order, USED, alloc_type);
    list_remove(&found->item);
    return found;
  } else {
    if (order == MAX_ORDER) {
      kprintf("We just ran out of buddy blocks. KernelAlloc=%d\n", alloc_type);
      /* kprintf("List[%d].length = %d\n", order,
              list_length(&buddy[order].free_list)); */

      return NULL;
    }
    /*     kprintf("Looking up at order %d\n", order + 1);
     */
    // Search in the higher order and split the block
    found = get_buddy_block(order + 1, alloc_type);
    if (found == NULL) {
      return NULL;
    }
    // I found a block in the higher order
    // Need to remove from free list, mark it used and add the 2 split blocks to
    // this free list
    found->order--;
    BuddyBlock *foundBuddy = find_buddy(found, alloc_type);
    foundBuddy->order = found->order;

    if (alloc_type == KERNEL_ALLOC)
      list_add_tail(&buddy[order].free_list, &foundBuddy->item);
    else if (alloc_type == FAST_ALLOC)
      list_add_tail(&fast_buddy[order].free_list, &foundBuddy->item);
    else
      list_add_tail(&normal_buddy[order].free_list, &foundBuddy->item);
    /*  kprintf("List[%d].length = %d\n", order + 1,
             list_length(&buddy[order + 1].free_list));
     kprintf("List[%d].length = %d\n", order,
             list_length(&buddy[order].free_list)); */

    set_block_usage(found, order, USED, alloc_type);
    set_block_usage(foundBuddy, order, FREE, alloc_type);

    // kprintf("Found block of higher order %d -> %x\n", order + 1, found);
    return found;
  }
}

BuddyBlock *find_buddy_order(BuddyBlock *me, int order, u8 alloc_type) {
  u32 block_pos = get_buddy_pos(me, alloc_type) / (PAGES_PER_BLOCK(order));
  // kprintf("Addr=%x Order=%d Pos=%d\n", me, order, block_pos);

  if (block_pos % 2 == 0) {
    // I am a block in an even position, my buddy is the next
    if (alloc_type == KERNEL_ALLOC)
      return &buddies[get_buddy_pos(me, alloc_type) + PAGES_PER_BLOCK(order)];
    else if (alloc_type == FAST_ALLOC)
      return &fast_buddies[get_buddy_pos(me, alloc_type) +
                           PAGES_PER_BLOCK(order)];
    else
      return &normal_buddies[get_buddy_pos(me, alloc_type) +
                             PAGES_PER_BLOCK(order)];
  }
  // Otherwise it is the previous
  if (alloc_type == KERNEL_ALLOC)
    return &buddies[get_buddy_pos(me, alloc_type) - PAGES_PER_BLOCK(order)];
  else if (alloc_type == FAST_ALLOC)
    return &fast_buddies[get_buddy_pos(me, alloc_type) -
                         PAGES_PER_BLOCK(order)];
  else
    return &normal_buddies[get_buddy_pos(me, alloc_type) -
                           PAGES_PER_BLOCK(order)];
}

BuddyBlock *find_buddy(BuddyBlock *me, u8 kernel_alloc) {
  return find_buddy_order(me, me->order, kernel_alloc);
}

void set_block_usage(BuddyBlock *p, int order, u8 used, u8 kernel_alloc) {
  int block_pos =
      get_pfn_from_page(p->head, kernel_alloc) / PAGES_PER_BLOCK(order);

  if (kernel_alloc == KERNEL_ALLOC)
    buddy[order].bitmap[block_pos] = used;
  else if (kernel_alloc == FAST_ALLOC)
    fast_buddy[order].bitmap[block_pos] = used;
  else
    normal_buddy[order].bitmap[block_pos] = used;
}

BuddyBlock *get_buddy_from_pos(int order, int pos, u8 alloc_type) {
  if (alloc_type == KERNEL_ALLOC)
    return (BuddyBlock *)(buddies + (pos * PAGES_PER_BLOCK(order)));
  else if (alloc_type == NORMAL_ALLOC)
    return (BuddyBlock *)(normal_buddies + (pos * PAGES_PER_BLOCK(order)));
  else
    return (BuddyBlock *)(fast_buddies + (pos * PAGES_PER_BLOCK(order)));
}

/*
  Returns the first free block in a given order or NULL
*/
BuddyBlock *search_free_block(int order, u8 alloc_type) {
  if (order < 0 || order > MAX_ORDER) return NULL;

  if (alloc_type == KERNEL_ALLOC) {
    u32 i = 0;
    for (i = 0; i < buddy[order].bitmap_length; ++i) {
      if (buddy[order].bitmap[i] == FREE) {
        return get_buddy_from_pos(order, i, alloc_type);
      }
    }
  } else if (alloc_type == NORMAL_ALLOC) {
    u32 i = 0;
    for (i = 0; i < normal_buddy[order].bitmap_length; ++i) {
      if (normal_buddy[order].bitmap[i] == FREE) {
        return get_buddy_from_pos(order, i, alloc_type);
      }
    }
  } else if (alloc_type == FAST_ALLOC) {
    u32 i = 0;
    for (i = 0; i < fast_buddy[order].bitmap_length; ++i) {
      if (fast_buddy[order].bitmap[i] == FREE) {
        return get_buddy_from_pos(order, i, alloc_type);
      }
    }
  }

  return NULL;
}
