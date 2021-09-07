#ifndef LIST_H
#define LIST_H

#include "../cpu/types.h"

typedef struct list_head {
  struct list_head *next;
  struct list_head *prev;
} List;

/*
  Given a pointer to an element of a struct (ptr),
  a struct type (type) and the name of the element of the struct (memeber)
  it returns the pointer to the struct containing it.
*/
#define container_of(ptr, type, member)                \
  ({                                                   \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
  })

/*
  Just an alias used to extract the struct from the embedded list pointer
 */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/*
  Init list element head by making it point to itself in both directions
*/
static inline void LIST_INIT(List *list) {
  list->next = list;
  list->prev = list;
}

#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
  for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

/*
 Adds element before the list head
 O(1) time complexity
*/
static inline void list_add_head(List *first, List *new) {
  List *second = first->next;
  second->prev = new;
  new->next = second;
  new->prev = first;
  first->next = new;
}

static inline void list_add_tail(List *first, List *new) {
  List *last = first->prev;
  last->next = new;
  new->next = first;
  new->prev = last;
  first->prev = new;
}

static inline int list_length(List *head) {
  if (head == NULL) return 0;
  List *p;
  int i = 0;
  list_for_each(p, head) { ++i; }
  return i;
}

static inline void list_remove(List *deleteme) {
  List *prev = deleteme->prev;
  List *next = deleteme->next;
  prev->next = next;
  next->prev = prev;
}
#endif