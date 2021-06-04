#ifndef BUDDY_H
#define BUDDY_H

#include "page.h"

#define MAX_ORDER 10
#define PAGES_PER_BLOCK(order) (1 << order)

#define FREE 1
#define USED 0

extern uint32_t total_free_memory;

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

void *address_from_pfn(uint32_t pfn);
uint32_t get_pfn_from_page(Page *p);
uint32_t get_pfn_from_address(void *);
Page *get_page_from_address(void *ptr);
void *get_page_address(Page *p);
BuddyBlock *find_buddy_order(BuddyBlock *me, int order);
BuddyBlock *find_buddy(BuddyBlock *me);
BuddyBlock *get_buddy_block(int order);
BuddyBlock *search_free_block(int order);
void free_buddy_block(BuddyBlock *b);
void set_block_usage(BuddyBlock *p, int order, int used);
void printBuddy(BuddyBlock *);
void buddy_init(Page *);

#endif