#ifndef MEM_H
#define MEM_H

#include "../mem/buddy.h"
#include "../boot/multiboot.h"
#include "mem_desc.h"

#define STACK_SIZE 0x1000
// 1048576

typedef struct boot_mmap {
  u32 highest_mem_addess;
  u32 total_pages;
} BootMmap;

extern Page *kernel_pages;
extern Page *normal_pages;

extern BootMmap boot_mmap;

extern byte
    *free_mem_addr;  // Reppresents the first byte that we can freeily allocate
extern byte *stack_pointer;  // Top of the kernel stack
extern u32 total_kernel_pages;

BuddyBlock *get_buddy_from_page(Page *p, u8 kernel_alloc);
void init_memory_ptrs();
void memcopy(byte *source, byte *dest, size_t nbytes);
void *boot_alloc(size_t size, uint8_t align);
void memset(byte *dest, u8 val, size_t len);
u8 parse_multiboot_info(KMultiBoot2Info *info);
void memory_alloc_init();
void *kalloc(u32 order);
void *normal_page_alloc(u32 order);
void kfree_normal(void *ptr);
void kfree(void *ptr);
void printFree();
void print_mem_desc(MemDesc *m);
#endif