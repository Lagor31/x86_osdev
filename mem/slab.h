#ifndef SLAB_H
#define SLAB_H
#include "../utils/list.h"



// Must always be contained in a order 0 buddy block
typedef struct slab {
  char size;
  List head;
  char first_free;
  char *free_blocks;
} Slab;


typedef struct mem_cache {
  Slab *free;
  Slab *used;
  Slab *empty;
} MemCache;

extern MemCache kMemCache;
Slab* createSlab(char size); 

#endif