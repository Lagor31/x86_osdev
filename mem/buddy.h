#ifndef BUDDY_H
#define BUDDY_H

#define MAX_ORDER 10
#define PAGES_PER_BLOCK(x) (1 << x)

typedef struct BuddyBlock BuddyBlock;

struct BuddyBlock {
  Page *head;
  uint8_t order;
  BuddyBlock *buddy;
  List list;
};

typedef struct buddy {
  uint8_t *bitmap;
  BuddyBlock *free_buddy_list;
  uint32_t bitmap_length;
} Buddy;

BuddyBlock *find_buddy_order(BuddyBlock *me, int order);
BuddyBlock *find_buddy(BuddyBlock *me);
BuddyBlock *get_buddy_block(int order);
BuddyBlock *search_free_block(int order);
void set_block_used(BuddyBlock *p, int order);

void buddy_init();

#endif