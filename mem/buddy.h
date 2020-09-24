#ifndef BUDDY_H
#define BUDDY_H

#define MAX_ORDER 10
#define PAGES_PER_BLOCK(x) (1 << x)

#define FREE 1
#define USED 0

typedef struct BuddyBlock BuddyBlock;

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

BuddyBlock *find_buddy_order(BuddyBlock *me, int order);
BuddyBlock *find_buddy(BuddyBlock *me);
BuddyBlock *get_buddy_block(int order);
BuddyBlock *search_free_block(int order);
void free_buddy_block(BuddyBlock *b);
void set_block_usage(BuddyBlock *p, int order, int used);
void printBuddy(BuddyBlock *);
void buddy_init();

#endif