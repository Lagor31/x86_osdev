#include "page.h"
#include "../cpu/types.h"

#include "buddy.h"

#include "../drivers/screen.h"
#include "../kernel/multiboot.h"

#include "../kernel/mem.h"
#include "../utils/list.h"

Buddy buddy[MAX_ORDER + 1];
BuddyBlock *buddies;

uint32_t get_pfn(Page *p) {
  return ((uint32_t)p - (uint32_t)pages) / sizeof(Page);
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
    LIST_INIT(&buddies[i].list);
  }

  for (curr_order = MAX_ORDER; curr_order >= 0; --curr_order) {
    if (curr_order == MAX_ORDER) {
      BuddyBlock *first = &buddies[0];
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
        list_add(&first->list, &buddies[curr_buddy_pos].list);
        if (i <= 3) {
          kprintf("%d : {Addr : %x, Page: %d, O: %d, Buddy: %x}\n",
                  curr_buddy_pos, &buddies[curr_buddy_pos],
                  get_pfn(buddies[curr_buddy_pos].head),
                  buddies[curr_buddy_pos].order, buddies[curr_buddy_pos].buddy);
        }
      }
      kprintf("\n");
      buddy[MAX_ORDER].free_buddy_list = first;
    } else {
      buddy[curr_order].free_buddy_list = NULL;
      number_of_blocks *= 2;
    }
    // Allocate bits, not bytes
    buddy[curr_order].bitmap =
        boot_alloc(sizeof(uint8_t) * number_of_blocks, 1);
    buddy[curr_order].bitmap_length = number_of_blocks;

    if (curr_order == MAX_ORDER)
      memset(buddy[MAX_ORDER].bitmap, 1, number_of_blocks);
    else
      memset(buddy[curr_order].bitmap, 0, number_of_blocks);
  }

  kprintf("Buddy order 0 of 0 : %x -> %x\n", &buddies[0],
          find_buddy_order(&buddies[0], 0));
  kprintf("Buddy order 0 of 1 : %x -> %x\n", &buddies[1],
          find_buddy_order(&buddies[1], 0));

  kprintf("Buddy order %d of 0 : %x -> %x\n", buddies[0].order, &buddies[0],
          find_buddy_order(&buddies[0], buddies[0].order));
  BuddyBlock *findMe = find_buddy(&buddies[0]);
  kprintf("Buddy order %d of 1 : %x -> %x\n", findMe->order, findMe,
          find_buddy_order(findMe, findMe->order));
}

BuddyBlock *get_buddy_block(int order) {
  BuddyBlock *found = search_free_block(order);
  if (found != NULL) {
    // Found a block in the free list, i need to mark it used and return it
    set_block_used(found, order);
    return found;
  } else {
    if (order == MAX_ORDER) {
      kprintf("We just ran out of buddy blocks.");
      return NULL;
    }
    // Search in the higher order and split the block
    found = get_buddy_block(order + 1);
    // I found a block in the higher order
    // Need to remove from free list, mark used and add the 2 split blocks to
    // this free list
    found->order /= 2;
    list_remove(&found->list, &buddy[order + 1].free_buddy_list->list);
    list_add(&buddy[order].free_buddy_list->list, &found->list);
    list_add(&buddy[order].free_buddy_list->list, &found->buddy->list);
  }

  return buddy[0].free_buddy_list->buddy;
}

int get_buddy_pos(BuddyBlock *b) {
  return ((uint32_t)b - (uint32_t)buddies) / sizeof(BuddyBlock);
}

BuddyBlock *find_buddy_order(BuddyBlock *me, int order) {
  int block_pos = get_buddy_pos(me) / (PAGES_PER_BLOCK(order));
  kprintf("Addr=%x Order=%d Pos=%d\n", me, order, block_pos);

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

void set_block_used(BuddyBlock *p, int order) {
  int block_pos = get_pfn(p->head) / PAGES_PER_BLOCK(order);
  buddy[order].bitmap[block_pos] = 0;
}

/*
  Returns the first free page in a given order or NULL
*/
BuddyBlock *search_free_block(int order) {
  if (order < 0 || order > MAX_ORDER) return NULL;

  if (buddy[order].free_buddy_list != NULL)
    return buddy[order].free_buddy_list;
  else
    return NULL;
}
