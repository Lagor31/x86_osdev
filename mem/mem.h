#ifndef MEM_H
#define MEM_H

#include "../mem/buddy.h"
#include "../boot/multiboot.h"
#include "mem_desc.h"

#define STACK_SIZE 0x1000
// 1048576

#define PAGES_PER_BLOCK(order) (1 << order)

#define FREE 1
#define USED 0

typedef struct boot_mmap {
  u32 highest_mem_addess;
  u32 total_pages;
} BootMmap;


extern BootMmap boot_mmap;

extern byte
    *free_mem_addr;  // Reppresents the first byte that we can freeily allocate
extern byte *stack_pointer;  // Top of the kernel stack
extern u32 total_kernel_pages;

extern u32 allocs_done;
extern u32 total_packets;
extern u32 allocs_size;

extern u32 total_used_memory;
extern u32 total_fused_memory;
extern u32 total_kused_memory;
extern u32 total_nused_memory;

void init_memory_ptrs();
void memcopy(byte *source, byte *dest, size_t nbytes);
void *boot_alloc(size_t size, uint8_t align);
void memset(byte *dest, u8 val, size_t len);
u8 parse_multiboot_info(KMultiBoot2Info *info);
void init_memory_alloc();
void *kalloc_page(u32 order);

void *kmalloc(u32 size);
void kfree(void *b);

void kfree_normal(void *ptr);

void *fmalloc(u32 size);
void ffree(void *);

void *kalloc_nosleep(u32 order);
void kfree_page(void *ptr);
void printFree();
void print_mem_desc(MemDesc *m);
u32 calc_page_order(u32 byte_size);

#endif