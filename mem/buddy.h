#ifndef BUDDY_H
#define BUDDY_H

#include "page.h"

#define MAX_ORDER 10
#define PAGES_PER_BLOCK(order) (1 << order)

#define FREE 1
#define USED 0

extern uint32_t total_used_memory;
extern uint32_t total_kused_memory;
extern uint32_t total_nused_memory;
extern uint32_t total_fused_memory;

extern uint32_t phys_normal_offset;
extern uint32_t phys_fast_offset;

void print_buddy_usage();

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
extern BuddyBlock *normal_buddies;
extern BuddyBlock *fast_buddies;
extern Buddy buddy[MAX_ORDER + 1];
extern Buddy normal_buddy[MAX_ORDER + 1];
extern Buddy fast_buddy[MAX_ORDER + 1];

void *phys_from_pfn(uint32_t pfn, uint8_t kernel_alloc);
uint32_t get_pfn_from_page(Page *p, uint8_t kernel_alloc);
uint32_t get_pfn_from_address(void *, uint8_t kernel_alloc);
Page *get_page_from_address(void *ptr, uint8_t kernel_alloc);
void *get_page_phys_address(Page *p, uint8_t kernel_alloc);
BuddyBlock *find_buddy_order(BuddyBlock *me, int order, uint8_t kernel_alloc);
BuddyBlock *find_buddy(BuddyBlock *me, uint8_t kernel_alloc);
BuddyBlock *get_buddy_block(int order, uint8_t kernel_alloc);
BuddyBlock *search_free_block(int order, uint8_t kernel_alloc);
void free_buddy_block(BuddyBlock *b, uint8_t kernel_alloc);
void set_block_usage(BuddyBlock *p, int order, uint8_t used,
                     uint8_t kernel_alloc);
void printBuddy(BuddyBlock *, uint8_t kernel_alloc);
void buddy_init(Page **, BuddyBlock **, Buddy *, uint32_t size);
uint32_t get_buddy_pos(BuddyBlock *b, uint8_t kernel_alloc);

#endif