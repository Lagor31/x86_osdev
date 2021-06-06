#ifndef BUDDY_H
#define BUDDY_H

#include "page.h"

#define MAX_ORDER 10
#define PAGES_PER_BLOCK(order) (1 << order)

#define FREE 1
#define USED 0

extern uint32_t total_free_memory;
extern uint32_t total_kfree_memory;
extern uint32_t total_nfree_memory;
extern uint32_t phys_normal_offset;

typedef struct buddy_block {
  Page *head;
  uint8_t order;
  List item;
} BuddyBlock;

typedef struct buddy {
  uint8_t *bitmap;
  List free_list;
  uint32_t bitmap_length;
} Buddy;

extern BuddyBlock *buddies;
extern Buddy buddy[MAX_ORDER + 1];
extern Buddy normal_buddy[MAX_ORDER + 1];
extern BuddyBlock *normal_buddies;
void *address_from_pfn(uint32_t pfn, uint8_t kernel_alloc);
uint32_t get_pfn_from_page(Page *p, uint8_t kernel_alloc);
uint32_t get_pfn_from_address(void *, uint8_t kernel_alloc);
Page *get_page_from_address(void *ptr, uint8_t kernel_alloc);
void *get_page_address(Page *p, uint8_t kernel_alloc);
BuddyBlock *find_buddy_order(BuddyBlock *me, int order, uint8_t kernel_alloc);
BuddyBlock *find_buddy(BuddyBlock *me, uint8_t kernel_alloc);
BuddyBlock *get_buddy_block(int order, uint8_t kernel_alloc);
BuddyBlock *search_free_block(int order, uint8_t kernel_alloc);
void free_buddy_block(BuddyBlock *b, uint8_t kernel_alloc);
void set_block_usage(BuddyBlock *p, int order, int used, uint8_t kernel_alloc);
void printBuddy(BuddyBlock *, uint8_t kernel_alloc);
void buddy_init(Page **, BuddyBlock **, Buddy *, uint32_t size,
                uint8_t kernel_alloc);

#endif