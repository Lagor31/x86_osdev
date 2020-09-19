#ifndef MEM_H
#define MEM_H

#define STACK_SIZE 0x1000
#define PHYS_MEM_FRAMES 100 
//1048576

typedef struct Page {
  // 8 Bits for various flags
  char buddy_info;
  // Usage count
  int count;
  // Free pages list for buddy allocator
  List list;
} Page;

extern struct Page *frames;

extern uint8_t
    *free_mem_addr;  // Reppresents the first byte that we can freeily allocate
extern uint8_t *stack_pointer;  // Top of the kernel stack

void meminit();
void memcopy(uint8_t *source, uint8_t *dest, size_t nbytes);
void *boot_alloc(size_t size, uint8_t align);
void memset(uint8_t *dest, uint8_t val, size_t len);

#endif