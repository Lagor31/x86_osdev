#ifndef BUDDY_H
#define BUDDY_H


#define MAX_ORDER 10

typedef struct buddy{
    void *bitmap;
    Page *free_buddy_list;
} Buddy;

void buddy_init();

#endif