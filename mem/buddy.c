#include "page.h"
#include "../cpu/types.h"

#include "buddy.h"
#include "../kernel/kernel.h"

#include "../drivers/screen.h"
#include "../kernel/multiboot.h"

#include "../kernel/mem.h"
#include "../utils/list.h"

Buddy buddy[MAX_ORDER + 1];
BuddyBlock *buddies;

uint32_t get_pfn(Page *p) {
  return ((uint32_t)p - (uint32_t)pages) / sizeof(Page);
}

void *pa(uint32_t pfn) {
  return (void *)(pfn * PAGE_SIZE + KERNEL_VIRTUAL_ADDRESS_BASE);
}

void *get_free_address(BuddyBlock *b) {
  return (BuddyBlock *)pa(get_pfn(b->head));
}

void printBuddy(BuddyBlock *b) {
  kprintf("[%x]->%x S=%d\n", b, get_free_address(b), b->order);
}

int get_buddy_pos(BuddyBlock *b) {
  return ((uint32_t)b - (uint32_t)buddies) / sizeof(BuddyBlock);
}

void buddy_init() {
  // Allocating array of pages
  pages = boot_alloc(sizeof(Page) * boot_mmap.total_pages, 1);
  int curr_order = MAX_ORDER;
  uint32_t number_of_blocks =
      boot_mmap.total_pages / PAGES_PER_BLOCK(MAX_ORDER);
  // blocks_number &= 0xFFFFFFFE;
  buddies = boot_alloc(sizeof(BuddyBlock) * boot_mmap.total_pages, 1);

  uint32_t i = 0;
  for (i = 0; i < boot_mmap.total_pages; ++i) {
    buddies[i].head = pages + i;
  }

  for (curr_order = MAX_ORDER; curr_order >= 0; --curr_order) {
    LIST_INIT(&buddy[curr_order].free_list);

    if (curr_order == MAX_ORDER) {
      kprintf("Order %d Total Blocks %d\n", curr_order, number_of_blocks);
      for (i = 0; i < number_of_blocks; ++i) {
        int curr_buddy_pos = i * PAGES_PER_BLOCK(MAX_ORDER);
        buddies[curr_buddy_pos].order = MAX_ORDER;
        // Find his buddy
        BuddyBlock *myBuddy;
        if (i % 2 == 0)
          myBuddy = &buddies[curr_buddy_pos + PAGES_PER_BLOCK(MAX_ORDER)];
        else
          myBuddy = &buddies[curr_buddy_pos - PAGES_PER_BLOCK(MAX_ORDER)];

        buddies[curr_buddy_pos].buddy = myBuddy;
        list_add(&buddy[curr_order].free_list, &buddies[curr_buddy_pos].item);
        if (i <= 3) {
          kprintf("%d : {Addr : %x, Page: %d, O: %d, Buddy: %x}\n",
                  curr_buddy_pos, &buddies[curr_buddy_pos],
                  get_pfn(buddies[curr_buddy_pos].head),
                  buddies[curr_buddy_pos].order, buddies[curr_buddy_pos].buddy);
        }
      }
      kprintf("\n");
    } else {
      number_of_blocks *= 2;
    }
    // Allocate bits, not bytes
    buddy[curr_order].bitmap =
        boot_alloc(sizeof(uint8_t) * number_of_blocks, 1);
    buddy[curr_order].bitmap_length = number_of_blocks;

    if (curr_order == MAX_ORDER)
      memset(buddy[MAX_ORDER].bitmap, FREE, number_of_blocks);
    else
      memset(buddy[curr_order].bitmap, USED, number_of_blocks);
  }
}

uint8_t is_buddy_block_free(BuddyBlock *b) {
  return buddy[b->order].bitmap[get_buddy_pos(b)];
}

void free_buddy_block(BuddyBlock *b) {
  kprintf("Freeing %x O:%d\n", b, b->order);
  BuddyBlock *my_buddy = find_buddy(b);
  uint8_t need_to_merge =
      is_buddy_block_free(my_buddy) && (b->order != MAX_ORDER);
  // need to merge and put up
  if (need_to_merge) {
    // I set myself and my buddy as used
    set_block_usage(b, b->order, USED);
    set_block_usage(my_buddy, my_buddy->order, USED);
    list_remove(&b->item);
    list_remove(&my_buddy->item);
    uint8_t higher_order = b->order + 1;
    b->order = higher_order;
    list_add(&buddy[higher_order].free_list, &b->item);
    set_block_usage(b, b->order, FREE);
  } else {
    // Adding block to its free list
    list_add(&buddy[b->order].free_list, &b->item);
    // Mark it free
    set_block_usage(b, b->order, FREE);
  }
}

BuddyBlock *get_buddy_block(int order) {
  int cur_list_length = 0;
  cur_list_length = list_length(&buddy[order].free_list);
  /*  kprintf("Looking for a block of order %d, List-length=%d ", order,
           cur_list_length); */
  BuddyBlock *found = search_free_block(order);
  if (found != NULL) {
    // Found a block in the free list, i need to mark it used and return it
    kprintf("\nFound block of order %d -> %x, ListSize=%d\n", order, found,
            list_length(&buddy[order].free_list));
    kprintf("Ptr Addr=%x Size=%d\n", get_free_address(found), found->order);

    set_block_usage(found, order, USED);
    list_remove(&found->item);

    return found;
  } else {
    if (order == MAX_ORDER) {
      kprintf("We just ran out of buddy blocks.");
      kprintf("List[%d].length = %d\n", order,
              list_length(&buddy[order].free_list));

      return NULL;
    }
    /*     kprintf("Looking up at order %d\n", order + 1);
     */    // Search in the higher order and split the block
    found = get_buddy_block(order + 1);
    // I found a block in the higher order
    // Need to remove from free list, mark used and add the 2 split blocks to
    // this free list
    found->order--;
    BuddyBlock *foundBuddy = find_buddy(found);
    foundBuddy->order = found->order;

    // list_add(&buddy[order].free_buddy_list->list, &found->list);
    list_add(&buddy[order].free_list, &foundBuddy->item);
    /*  kprintf("List[%d].length = %d\n", order + 1,
             list_length(&buddy[order + 1].free_list));
     kprintf("List[%d].length = %d\n", order,
             list_length(&buddy[order].free_list)); */

    set_block_usage(found, order, USED);
    set_block_usage(foundBuddy, order, FREE);
    // kprintf("Found block of higher order %d -> %x\n", order + 1, found);
    return found;
  }
}

BuddyBlock *find_buddy_order(BuddyBlock *me, int order) {
  int block_pos = get_buddy_pos(me) / (PAGES_PER_BLOCK(order));
  // kprintf("Addr=%x Order=%d Pos=%d\n", me, order, block_pos);

  if (block_pos % 2 == 0) {
    // I am a block in an even position, my buddy is the next
    return &buddies[get_buddy_pos(me) + PAGES_PER_BLOCK(order)];
  }
  // Otherwise it is the previous
  return &buddies[get_buddy_pos(me) - PAGES_PER_BLOCK(order)];
}

BuddyBlock *find_buddy(BuddyBlock *me) {
  return find_buddy_order(me, me->order);
}

void set_block_usage(BuddyBlock *p, int order, int used) {
  int block_pos = get_pfn(p->head) / PAGES_PER_BLOCK(order);
  buddy[order].bitmap[block_pos] = used;
}

BuddyBlock *get_buddy_from_pos(int order, int pos) {
  return (BuddyBlock*) (buddies + (pos * PAGES_PER_BLOCK(order)));
}
/*
  Returns the first free block in a given order or NULL
*/
BuddyBlock *search_free_block(int order) {
  if (order < 0 || order > MAX_ORDER) return NULL;

  int i = 0;
  for (i = 0; i < buddy[order].bitmap_length; ++i) {
    if (buddy[order].bitmap[i] == FREE) {
      return get_buddy_from_pos(order, i);
    }
  }
  return NULL;
}
