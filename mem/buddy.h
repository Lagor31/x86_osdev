#ifndef BUDDY_H
#define BUDDY_H

#include "page.h"

#define MAX_ORDER 10
#define PAGES_PER_BLOCK(x) (1 << x)

#define FREE 1
#define USED 0

typedef struct BuddyBlock BuddyBlock;
extern uint32_t total_free_memory;
extern BuddyBlock *buddies;

struct BuddyBlock {
  Page *head;
  uint8_t order;
  List item;
};

typedef struct buddy {
  uint8_t *bitmap;
  List free_list;
  uint32_t bitmap_length;
} Buddy;

void *ka(uint32_t pfn);
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
void buddy_init();

#endif