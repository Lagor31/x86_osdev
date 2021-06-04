#ifndef MEM_H
#define MEM_H

#include "../mem/buddy.h"
#define STACK_SIZE 0x1000
#define PHYS_MEM_FRAMES 100
// 1048576

typedef struct boot_mmap {
  uint32_t highest_mem_addess;
  uint32_t total_pages;
} BootMmap;

extern Page *kernel_pages;
extern BootMmap boot_mmap;
extern void *pages_base_addr;

extern uint8_t
    *free_mem_addr;  // Reppresents the first byte that we can freeily allocate
extern uint8_t *stack_pointer;  // Top of the kernel stack

BuddyBlock *get_buddy_from_page(Page *p);
void init_memory_subsystem();
void memcopy(uint8_t *source, uint8_t *dest, size_t nbytes);
void *boot_alloc(size_t size, uint8_t align);
void memset(uint8_t *dest, uint8_t val, size_t len);
uint8_t parse_multiboot_info(struct kmultiboot2info *info);
void memory_alloc_init();
void *kmalloc(uint32_t order);
void printFree();
#endif