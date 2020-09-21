#ifndef LIST_H
#define LIST_H

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

/*
 Adds element before the list head
 O(1) time complexity
*/
static inline void list_add(List *first, List *new) {
  List *last = first->prev;
  last->next = new;
  new->next = first;
  new->prev = last;
  first->prev = new;
}

#endif