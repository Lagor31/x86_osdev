#ifndef USER_H
#define USER_H

#include "../lib/list.h"
#include "../mem/mem.h"

extern List users;

typedef struct user_t {
  u32 uid;
  char *username;
  List kuserq;
} User;

extern User *root;

extern void init_users();
#endif