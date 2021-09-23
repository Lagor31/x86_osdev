#include "../cpu/types.h"

#include "../drivers/screen.h"
#include "../kernel/kernel.h"

#include "../lib/list.h"
#include "../lib/utils.h"
#include "../boot/multiboot.h"
#include "page.h"
#include "buddy.h"
#include "slab.h"
#include "vma.h"
#include "mem.h"

u8 *free_mem_addr;  // Represents the first byte that we can freely allocate
u8 *stack_pointer;  // Top of the kernel stack

uint32_t total_kernel_pages = 0;
uint32_t total_normal_pages = 0;
uint32_t total_fast_pages = 0;

Page *kernel_pages;
Page *normal_pages;
Page *fast_pages;

BootMmap boot_mmap;

void print_mem_desc(MemDesc *m) {
  List *l;
  VMArea *area;
  list_for_each(l, &m->vm_areas) {
    area = list_entry(l, VMArea, head);
    kprintf("S: 0x%x, E: 0x%x, S: 0x%x, PS: 0x%x F: 0x%d\n", area->start,
            area->end, area->size, area->phys_start, area->flags);
  }
}

void printFree() {
  int totFree = total_used_memory / 1024 / 1024;
  int tot = boot_mmap.total_pages * 4096 / 1024 / 1024;
  uint32_t tot_kern_size_mb = total_kernel_pages * 4096 / 1024 / 1024;
  uint32_t tot_norm_size_mb = total_normal_pages * 4096 / 1024 / 1024;
  uint32_t tot_fast_size_mb = total_fast_pages * 4096 / 1024 / 1024;

  kprintf("Used: %d / %d Mb\n\n", totFree, tot);
  kprintf("Kernel Mem used: %d/%d\n", total_kused_memory / 1024 / 1024,
          tot_kern_size_mb);
  kprintf("Normal Mem used: %d/%d\n", total_nused_memory / 1024 / 1024,
          tot_norm_size_mb);
  kprintf("Fast Mem used: %d/%d\n", total_fused_memory / 1024 / 1024,
          tot_fast_size_mb);
}
/* Getting the _stack_address as set in assembly to denote the beginning of
 * freeily allocatable memory */
void init_memory_ptrs() {
  stack_pointer = (u8 *)_stack_address;
  kprintf("Stack Pointer: 0x%x\n", (uint32_t)stack_pointer);
  free_mem_addr = stack_pointer;
  kprintf("Free memory address: 0x%x\n", (uint32_t)free_mem_addr);
}
/*
  Returns the pointer of the first available page with the specified order of
  free memory
*/
Page *alloc_kernel_pages(int order) {
  BuddyBlock *b = get_buddy_block(order, KERNEL_ALLOC);
  // printBuddy(b, KERNEL_ALLOC);
  if (b == NULL) {
    return NULL;
  }
  u32 allocated = PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  total_used_memory += allocated;
  total_kused_memory += allocated;

  return b->head;
}

Page *alloc_kernel_pages_nosleep(int order) {
  BuddyBlock *b = get_buddy_block(order, KERNEL_ALLOC);
  // printBuddy(b, KERNEL_ALLOC);
  if (b == NULL) return NULL;
  u32 allocated = PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  total_used_memory += allocated;
  total_kused_memory += allocated;
  return b->head;
}

Page *alloc_normal_pages(int order) {
  get_lock(nmem_lock);

  BuddyBlock *b = get_buddy_block(order, NORMAL_ALLOC);
  // printBuddy(b, NORMAL_ALLOC);
  if (b == NULL) {
    unlock(nmem_lock);
    return NULL;
  }
  u32 allocated = PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  total_used_memory += allocated;
  total_nused_memory += allocated;
  unlock(nmem_lock);
  return b->head;
}

Page *alloc_fast_pages(int order) {
  bool pi = disable_int();

  BuddyBlock *b = get_buddy_block(order, FAST_ALLOC);
  enable_int(pi);
  // printBuddy(b, NORMAL_ALLOC);
  if (b == NULL) {
    return NULL;
  }
  u32 allocated = PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  total_used_memory += allocated;
  total_fused_memory += allocated;
  return b->head;
}

void free_kernel_pages(Page *p) {
  // kprintf(" Free K PN %d\n", get_pfn_from_page(p, KERNEL_ALLOC));
  BuddyBlock *b = get_buddy_from_page(p, KERNEL_ALLOC);
  u32 released = PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  total_kused_memory -= released;
  total_used_memory -= released;
  get_lock(kmem_lock);
  free_buddy_block(b, KERNEL_ALLOC);
  unlock(kmem_lock);
}

void free_normal_pages(Page *p) {
  BuddyBlock *b = get_buddy_from_page(p, NORMAL_ALLOC);
  u32 released = PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  total_nused_memory -= released;
  total_used_memory -= released;
  // kprintf(" Free N PN %d\n", get_pfn_from_page(p, NORMAL_ALLOC));
  get_lock(nmem_lock);
  free_buddy_block(b, NORMAL_ALLOC);
  unlock(nmem_lock);
}
void free_fast_pages(Page *p) {
  BuddyBlock *b = get_buddy_from_page(p, FAST_ALLOC);
  u32 released = PAGES_PER_BLOCK(b->order) * PAGE_SIZE;
  total_fused_memory -= released;
  total_used_memory -= released;
  // kprintf(" Free N PN %d\n", get_pfn_from_page(p, NORMAL_ALLOC));
  bool pi = disable_int();
  free_buddy_block(b, FAST_ALLOC);
  enable_int(pi);
}

void *kmalloc(u32 size) {
  if (size <= MAX_SLAB_SIZE) {
    // Search in cache or create one ad hoc
    if (size < 4) size = 4;

    void *out = salloc(size);
    if (out != NULL) return out;

    // kprintf("Not found  in cache %d,  creating...\n", size);
    createSlab(size, FALSE);
    out = salloc(size);
    if (out == NULL) kprintf("NULL after cache creation\n");
    return out;
  } else {
    u32 order = 0;
    u32 pages = size / PAGE_SIZE + ((size % PAGE_SIZE) > 0 ? 1 : 0);

    while (order <= MAX_ORDER) {
      if (pages <= (u32)PAGES_PER_BLOCK(order)) {
        // kprintf("Size %d -> Order: %d\n", size, order);
        return kalloc_page(order);
      }
      order++;
    }
    return NULL;
  }
}

void kfree(void *buf) {
  if (!sfree(buf)) {
    kprintf("Error freeing cache obj");
  }
}

void *nmalloc(u32 size) {
  u32 order = 0;
  u32 pages = size / PAGE_SIZE + ((size % PAGE_SIZE) > 0 ? 1 : 0);

  while (order <= MAX_ORDER) {
    if (pages <= (u32)PAGES_PER_BLOCK(order)) {
      // kprintf("Size %d -> Order: %d\n", size, order);
      return normal_page_alloc(order);
    }
    order++;
  }
  return NULL;
}

void *kalloc_page(u32 order) {
  get_lock(kmem_lock);
  Page *p = alloc_kernel_pages(order);
  unlock(kmem_lock);
  if (p == NULL) return NULL;
  return get_page_phys_address(p, KERNEL_ALLOC) + KERNEL_VIRTUAL_ADDRESS_BASE;
}

void *kalloc_nosleep(u32 order) {
  Page *p = alloc_kernel_pages_nosleep(order);
  if (p == NULL) return NULL;
  return get_page_phys_address(p, KERNEL_ALLOC) + KERNEL_VIRTUAL_ADDRESS_BASE;
}

void *fmalloc(u32 size) {
  u32 order = 0;
  u32 pages = size / PAGE_SIZE + ((size % PAGE_SIZE) > 0 ? 1 : 0);

  while (order <= MAX_ORDER) {
    if (pages <= (u32)PAGES_PER_BLOCK(order)) {
      // kprintf("Size %d -> Order: %d\n", size, order);

      Page *p = alloc_fast_pages(order);
      if (p == NULL) return NULL;
      return get_page_phys_address(p, FAST_ALLOC) + KERNEL_VIRTUAL_ADDRESS_BASE;
    }
    order++;
  }
  return NULL;
}

// Returns a raw phys address
void *normal_page_alloc(u32 order) {
  Page *p = alloc_normal_pages(order);
  if (p == NULL) return NULL;
  return get_page_phys_address(p, NORMAL_ALLOC);
}

void kfree_page(void *ptr) {
  if (ptr == NULL) return;
  // kprintf("Free ptr %x ", ptr);
  free_kernel_pages(get_page_from_address(ptr, KERNEL_ALLOC));
}
// Must free a phys address
void kfree_normal(void *ptr) {
  if (ptr == NULL) return;
  // kprintf("Free ptr %x ", ptr);
  free_normal_pages(get_page_from_address(ptr, NORMAL_ALLOC));
}
void ffree(void *p) {
  if (p == NULL) return;
  // kprintf("Free ptr %x ", ptr);
  free_fast_pages(get_page_from_address(p, FAST_ALLOC));
}
BuddyBlock *get_buddy_from_page(Page *p, u8 alloc_type) {
  if (alloc_type == KERNEL_ALLOC)
    return (BuddyBlock *)(buddies + get_pfn_from_page(p, alloc_type));
  else if (alloc_type == FAST_ALLOC)
    return (BuddyBlock *)(fast_buddies + get_pfn_from_page(p, alloc_type));
  else
    return (BuddyBlock *)(normal_buddies + get_pfn_from_page(p, alloc_type));
}

void init_memory_alloc() {
  total_kernel_pages = (boot_mmap.total_pages / KERNEL_RATIO);
  total_fast_pages = (boot_mmap.total_pages / FAST_MEM_RATIO);
  total_normal_pages =
      boot_mmap.total_pages - total_kernel_pages - total_fast_pages;
  kprintf("Total kernel pages %d\nTotal normal pages %d\nTotal fast pages %d\n",
          total_kernel_pages, total_normal_pages, total_fast_pages);
  kprintf("Alloc kernel buddy system\n");
  buddy_init(&kernel_pages, &buddies, buddy, total_kernel_pages);

  phys_fast_offset = (total_kernel_pages)*PAGE_SIZE;
  kprintf("Physical fast offset : 0x%x\n", phys_fast_offset);
  kprintf("Alloc fast buddy system\n");
  buddy_init(&fast_pages, &fast_buddies, fast_buddy, total_fast_pages);

  phys_normal_offset = (total_kernel_pages + total_fast_pages) * PAGE_SIZE;
  kprintf("Physical normal offset : 0x%x\n", phys_normal_offset);
  kprintf("Alloc normal buddy system\n");
  buddy_init(&normal_pages, &normal_buddies, normal_buddy, total_normal_pages);

  kprintf("Total free memory=%dMb\n", total_used_memory / 1024 / 1024);

  int i = 0;
  int firstNUsedPages = ((u32)PA(free_mem_addr) / PAGE_SIZE) + 1;
  int four_megs_pages = firstNUsedPages / PAGES_PER_BLOCK(10);

  kprintf("You've used the first %d pages, allocating now %d 4Mb pages...\n",
          firstNUsedPages, ++four_megs_pages);
  for (i = 0; i < four_megs_pages; ++i) kalloc_nosleep(10);
  kprintf("Total free memory=%dMb\n", total_used_memory / 1024 / 1024);
  // After this, you can no longer use boot_alloc
}

u8 parse_multiboot_info(KMultiBoot2Info *info) {
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
void memcopy(byte *source, byte *dest, size_t nbytes) {
  size_t i;
  for (i = 0; i < nbytes; i++) {
    *(dest + i) = *(source + i);
  }
}

// Sets len byte to value val starting at address dest
void memset(u8 *dest, u8 val, size_t len) {
  u8 *temp = (u8 *)dest;
  for (; len != 0; len--) *temp++ = val;
}

/*
  Our memory allocator is just a pointer as of now.
  No way to free chunks (there are no chunks).
*/
void *boot_alloc(size_t size, u8 align) {
  // Pages are aligned to 4K, or 0x1000
  uint32_t isAligned = (uint32_t)free_mem_addr & 0x00000FFF;
  if (align == 1 && isAligned != 0) {
    free_mem_addr = (void *)((uint32_t)free_mem_addr & 0xFFFFF000);
    free_mem_addr += 0x1000;
  }
  // kprintf("Free mem pointer 0x%x\n", free_mem_addr);
  void *ret = free_mem_addr;
  // memset(ret, 0, size);   // Setting the newly allocated memory to 0
  free_mem_addr += size;  // We move up to the next free byte
  return ret;
}