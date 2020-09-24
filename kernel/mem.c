#include "../cpu/types.h"

#include "../drivers/screen.h"
#include "../kernel/kernel.h"

#include "../utils/list.h"
#include "../utils/utils.h"
#include "multiboot.h"
#include "../mem/page.h"
#include "../mem/buddy.h"
#include "mem.h"

uint8_t
    *free_mem_addr;  // Reppresents the first byte that we can freeily allocate
uint8_t *stack_pointer;  // Top of the kernel stack

Page *pages;
BootMmap boot_mmap;

/* Getting the _stack_address as set in assembly to denote the beginning of
 * freeily allocatable memory */
void meminit() {
  stack_pointer = (uint8_t *)_stack_address;
  kprintf("Stack Pointer: 0x%x\n", (uint32_t)stack_pointer);
  free_mem_addr = stack_pointer;
  kprintf("Free memory address: 0x%x\n", (uint32_t)free_mem_addr);
}

void memory_alloc_init() {
  int mem_system_size = (uint32_t)free_mem_addr;
  buddy_init();
  kprintf("Free memory address: 0x%x\n", (uint32_t)free_mem_addr);
  kprintf("Kernel Memory Subsystem Usage %d bytes\n",
          (uint32_t)free_mem_addr - mem_system_size);

  int i = 0;
  for (i = 0; i < 150; ++i) {
    BuddyBlock *b1 = get_buddy_block(9);
    if (b1 != NULL) {
      printBuddy(b1);
      //free_buddy_block(b1);
    }
  }
}

uint8_t parse_multiboot_info(struct kmultiboot2info *info) {
  struct multiboot_tag *tag;

  /*  Am I booted by a Multiboot-compliant boot loader? */
  if (info->magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
    kprintf("Invalid magic number: 0x%x\n", (unsigned)info->magic);
    return -1;
  }

  unsigned max_mem_address = 0;

  for (tag = (struct multiboot_tag *)(info->info + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag +
                                      ((tag->size + 7) & ~7))) {
    switch (tag->type) {
      case MULTIBOOT_TAG_TYPE_MMAP: {
        multiboot_memory_map_t *mmap;
        for (mmap = ((struct multiboot_tag_mmap *)tag)->entries;
             (multiboot_uint8_t *)mmap < (multiboot_uint8_t *)tag + tag->size;
             mmap = (multiboot_memory_map_t
                         *)((unsigned long)mmap +
                            ((struct multiboot_tag_mmap *)tag)->entry_size)) {
          unsigned base_addr = (unsigned)(mmap->addr >> 32) +
                               (unsigned)(mmap->addr & 0xffffffff);
          unsigned length =
              (unsigned)(mmap->len >> 32) + (unsigned)(mmap->len & 0xffffffff);

          unsigned type = (unsigned)mmap->type;
          if (type == 1 && (base_addr + length) > max_mem_address)
            max_mem_address = base_addr + length;
        }
      } break;
    }
  }
  boot_mmap.highest_mem_addess = max_mem_address;
  boot_mmap.total_pages = (max_mem_address & 0xFFFFF000) / PAGE_SIZE;
  kprintf("High mem addr: %x\n", boot_mmap.highest_mem_addess);
  kprintf("Total usable bytes %d\n", max_mem_address);
  kprintf("Total pages [0 - %d]\n", boot_mmap.total_pages);
  return 0;
}

// Just copies byte per byte from source to destination
void memcopy(uint8_t *source, uint8_t *dest, size_t nbytes) {
  size_t i;
  for (i = 0; i < nbytes; i++) {
    *(dest + i) = *(source + i);
  }
}

// Sets len byte to value val starting at address dest
void memset(uint8_t *dest, uint8_t val, size_t len) {
  uint8_t *temp = (uint8_t *)dest;
  for (; len != 0; len--) *temp++ = val;
}

/*
  Our memory allocator is just a pointer as of now.
  No way to free chunks (there are no chunks).
  TODO: Implement a (serious) memory allocator.
*/
void *boot_alloc(size_t size, uint8_t align) {
  // Pages are aligned to 4K, or 0x1000
  uint32_t isAligned = (uint32_t)free_mem_addr & 0x00000FFF;
  if (align == 1 && isAligned != 0) {
    free_mem_addr = (void *)((uint32_t)free_mem_addr & 0xFFFFF000);
    free_mem_addr += 0x1000;
  }
  // kprintf("Free mem pointer 0x%x\n", free_mem_addr);
  void *ret = free_mem_addr;
  memset(ret, 0, size);   // Setting the newly allocated memory to 0
  free_mem_addr += size;  // We move up to the next free byte
  return ret;
}