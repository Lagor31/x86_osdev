#include "user.h"
#include "../lib/strings.h"


List users;
u32 uid = 0;
User *root;

User *create_user(char *name) {
  User *u = (User *)kalloc(0);

  char *new_name = kalloc(0);
  u32 name_length = strlen(name);
  memcopy((byte *)name, (byte *)new_name, name_length);
  new_name[name_length] = '\0';

  u->username = new_name;
  u->uid = uid++;
  u->gid = 0;
  list_add_head(&users, &u->kuserq);
  return u;
}

void init_users() {
  LIST_INIT(&users);
  root = create_user("root");
}