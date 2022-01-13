#ifndef BUDDY_NEW_H
#define BUDDY_NEW_H
#include "buddy.h"
#include "../cpu/types.h"
#include "../lib/list.h"

typedef struct BuddyBlockNew BuddyBlockNew;

struct BuddyBlockNew {
  u8 order;
  bool free;
  u32 pfn;
  List free_list;
  BuddyBlockNew *my_buddy;
};

typedef struct buddy_mem_desc_t {
  u32 pfn_offset;
  u32 tot_pages;
  BuddyBlockNew* all_buddies;
  List free_lists[MAX_ORDER + 1];
} BuddyMemDesc;

extern BuddyMemDesc fast_buddy_new;
BuddyBlockNew* buddy_new_from_phys(u32 phys, BuddyMemDesc* mem_desc);
u32 calc_buddy_phys(BuddyBlockNew* b, BuddyMemDesc* mem_desc);
void init_buddy_new(u32 num_pages, u32 pfn_offset, BuddyMemDesc* buddy_test);
void print_buddy_status(u32 order, BuddyMemDesc*);
BuddyBlockNew* get_buddy_new(u32 order, BuddyMemDesc* buddy_test);
void print_buddy_new(BuddyBlockNew* b);
void free_buddy_new(BuddyBlockNew* b, BuddyMemDesc* buddy_test);
BuddyBlockNew* calc_free_buddy(BuddyBlockNew* b, BuddyMemDesc* buddy_test);
u32 calc_my_buddy_new_pfn(BuddyBlockNew* b, u32 order);
#endif