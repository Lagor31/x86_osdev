#include "../cpu/types.h"

#include "../drivers/screen.h"
#include "../kernel/kernel.h"

#include "../utils/list.h"
#include "../utils/utils.h"
#include "../boot/multiboot.h"
#include "../mem/page.h"
#include "../mem/buddy.h"
#include "mem.h"

#define KERNEL_ALLOC 1
#define NORMAL_ALLOC 0

uint8_t
    *free_mem_addr;  // Represents the first byte that we can freely allocate
uint8_t *stack_pointer;  // Top of the kernel stack

uint32_t total_kernel_pages = 0;
uint32_t total_normal_pages = 0;

Page *kernel_pages;
Page *normal_pages;

BootMmap boot_mmap;

void printFree() {
  int totFree = total_free_memory / 1024 / 1024;
  int tot = boot_mmap.total_pages * 4096 / 1024 / 1024;
  uint32_t tot_kern_size_mb = total_kernel_pages * 4096 / 1024 / 1024;
  uint32_t tot_norm_size_mb = total_normal_pages * 4096 / 1024 / 1024;
  uint32_t tot_free_kern_mb = total_kfree_memory / 1024 / 1024;
  uint32_t tot_free_norm_mb = total_nfree_memory / 1024 / 1024;

  kprintf("Free: %d / %d Mb\nUsed: %dMb\n", totFree, tot, tot - totFree);
  kprintf("KMem used: %d/%d\n", tot_kern_size_mb - tot_free_kern_mb,
          tot_kern_size_mb);
  kprintf("NMem used: %d/%d\n", tot_norm_size_mb - tot_free_norm_mb,
          tot_norm_size_mb);
}
/* Getting the _stack_address as set in assembly to denote the beginning of
 * freeily allocatable memory */
void init_memory_subsystem() {
  stack_pointer = (uint8_t *)_stack_address;
  kprintf("Stack Pointer: 0x%x\n", (uint32_t)stack_pointer);
  free_mem_addr = stack_pointer;
  kprintf("Free memory address: 0x%x\n", (uint32_t)free_mem_addr);
}
/*
  Returns the pointer of the first available page with the specified order of
  free memory
*/
Page *alloc_pages(int order) {
  BuddyBlock *b = get_buddy_block(order, KERNEL_ALLOC);
  printBuddy(b, KERNEL_ALLOC);
  if (b == NULL) return NULL;
  total_free_memory -= PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  total_kfree_memory -= PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  return b->head;
}

Page *alloc_normal_pages(int order) {
  BuddyBlock *b = get_buddy_block(order, NORMAL_ALLOC);
  printBuddy(b, NORMAL_ALLOC);
  if (b == NULL) return NULL;
  total_free_memory -= PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  total_nfree_memory -= PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  return b->head;
}

void free_pages(Page *p) {
  kprintf(" Free K PN %d\n", get_pfn_from_page(p, KERNEL_ALLOC));
  free_buddy_block(get_buddy_from_page(p, KERNEL_ALLOC), KERNEL_ALLOC);
}

void free_normal_pages(Page *p) {
  kprintf(" Free N PN %d\n", get_pfn_from_page(p, NORMAL_ALLOC));
  free_buddy_block(get_buddy_from_page(p, NORMAL_ALLOC), NORMAL_ALLOC);
}

void *kernel_page_alloc(uint32_t order) {
  Page *p = alloc_pages(order);
  if (p == NULL) return NULL;
  return get_page_phys_address(p, KERNEL_ALLOC) + KERNEL_VIRTUAL_ADDRESS_BASE;
}

void *normal_page_alloc(uint32_t order) {
  Page *p = alloc_normal_pages(order);
  if (p == NULL) return NULL;
  return get_page_phys_address(p, NORMAL_ALLOC) + KERNEL_VIRTUAL_ADDRESS_BASE;
}

void kfree(void *ptr) {
  if (ptr == NULL) return;
  kprintf("Free ptr %x ", ptr);
  free_pages(get_page_from_address(ptr, KERNEL_ALLOC));
}

void kfreeNormal(void *ptr) {
  if (ptr == NULL) return;
  kprintf("Free ptr %x ", ptr);
  free_normal_pages(
      get_page_from_address(ptr - KERNEL_VIRTUAL_ADDRESS_BASE, NORMAL_ALLOC));
}

BuddyBlock *get_buddy_from_page(Page *p, uint8_t kernel_alloc) {
  if (kernel_alloc)
    return (BuddyBlock *)(buddies + get_pfn_from_page(p, kernel_alloc));
  else
    return (BuddyBlock *)(normal_buddies + get_pfn_from_page(p, kernel_alloc));
}

void memory_alloc_init() {
  total_kernel_pages = (boot_mmap.total_pages / KERNEL_RATIO) + 1;
  total_normal_pages = boot_mmap.total_pages - total_kernel_pages;
  kprintf("Total kernel pages %d\nTotal normal pages %d\n", total_kernel_pages,
          total_normal_pages);
  kprintf("Alloc kernel buddy system\n");
  buddy_init(&kernel_pages, &buddies, buddy, total_kernel_pages, KERNEL_ALLOC);

  phys_normal_offset = total_kernel_pages * PAGE_SIZE;
  kprintf("Physical normal offset : 0x%x\n", phys_normal_offset);
  kprintf("Alloc normal buddy system\n");
  buddy_init(&normal_pages, &normal_buddies, normal_buddy, total_normal_pages,
             NORMAL_ALLOC);

  kprintf("Total free memory=%dMb\n", total_free_memory / 1024 / 1024);

  int i = 0;
  int firstNUsedPages = ((uint32_t)PA(free_mem_addr) / PAGE_SIZE) + 1;
  int four_megs_pages = firstNUsedPages / PAGES_PER_BLOCK(10);

  kprintf("You've used the first %d pages allocating now %d 4Mb pages...\n",
          firstNUsedPages, ++four_megs_pages);
  for (i = 0; i < four_megs_pages; ++i) kernel_page_alloc(10);
  kprintf("Total free memory=%dMb\n", total_free_memory / 1024 / 1024);
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
  kprintf("Total usable bytes %u\n", max_mem_address);
  kprintf("Total pages [0 - %u]\n", boot_mmap.total_pages);
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