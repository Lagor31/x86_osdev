#ifndef FDLIST_H
#define FDLIST_H

#include "../lib/list.h"

typedef struct FD FD;

typedef struct fd_list {
  FD *fd;
  List q;
} FDList;

#endif