#include "../cpu/types.h"

#include "../drivers/screen.h"
#include "../kernel/kernel.h"

#include "../lib/list.h"
#include "../lib/utils.h"
#include "../boot/multiboot.h"
#include "page.h"
#include "slab.h"
#include "vma.h"
#include "mem.h"
#include "buddy.h"

u8 *free_mem_addr;  // Represents the first byte that we can freely allocate
u8 *stack_pointer;  // Top of the kernel stack

uint32_t total_kernel_pages = 0;
uint32_t total_normal_pages = 0;
uint32_t total_fast_pages = 0;

u32 total_used_memory;
u32 total_fused_memory;
u32 total_kused_memory;
u32 total_nused_memory;

u32 allocs_done = 0;
u32 allocs_size = 0;
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

void *kmalloc(u32 size) {
  goto no_cache;
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
  }

  else {
  no_cache:
    /*  u32 order = 0;
     u32 pages = size / PAGE_SIZE + ((size % PAGE_SIZE) > 0 ? 1 : 0);

     while (order <= MAX_ORDER) {
       if (pages <= (u32)PAGES_PER_BLOCK(order)) {
         // kprintf("Size %d -> Order: %d\n", size, order);
         return kalloc_page(order);
       }
       order++;
     } */

    return kalloc_page(calc_page_order(size));
  }
}

void kfree(void *buf) {
  kfree_page(buf);
  return;
  if (!sfree(buf)) {
    kprintf("Error freeing cache obj");
  }
}

void *fmalloc(u32 size) {
  void *outP = NULL;
  u32 order = calc_page_order(size);
  BuddyBlockNew *p = get_buddy_new(order, &fast_buddy_new);
  outP = (void *)VA((u32)calc_buddy_phys(p, &fast_buddy_new));
  // kprintf("Alloc ptr: 0x%x Buddy: ", outP);
  // print_buddy_new(p);
  u32 allocated = (PAGES_PER_BLOCK(p->order) * PAGE_SIZE);
  total_fused_memory += allocated;
  total_used_memory += allocated;
  return outP;
}

void ffree(void *p) {
  if (p == NULL) return;
  // kprintf("Free ptr %x ", p);
  u32 phys = PA((u32)p);
  BuddyBlockNew *free_me = buddy_new_from_phys(phys, &fast_buddy_new);
  // kprintf("Buddy: ");
  // print_buddy_new(free_me);
  u32 freed = (PAGES_PER_BLOCK(free_me->order) * PAGE_SIZE);
  free_buddy_new(free_me, &fast_buddy_new);
  total_fused_memory -= freed;
  total_used_memory -= freed;
}

void *kalloc_page(u32 order) {
  void *outP = NULL;
  get_lock(kmem_lock);
  BuddyBlockNew *p = get_buddy_new(order, &kernel_buddy_new);
  outP = (void *)VA((u32)calc_buddy_phys(p, &kernel_buddy_new));
  // kprintf("Alloc ptr: 0x%x Buddy: ", outP);
  // print_buddy_new(p);
  u32 allocated = PAGES_PER_BLOCK(p->order) * PAGE_SIZE;
  total_kused_memory += allocated;
  total_used_memory += allocated;

  unlock(kmem_lock);
  return outP;
}

void *kalloc_nosleep(u32 order) {
  void *outP = NULL;
  BuddyBlockNew *p = get_buddy_new(order, &kernel_buddy_new);
  outP = (void *)VA((u32)calc_buddy_phys(p, &kernel_buddy_new));
  // kprintf("Alloc ptr: 0x%x Buddy: ", outP);
  // print_buddy_new(p);
  u32 allocated = PAGES_PER_BLOCK(p->order) * PAGE_SIZE;
  total_kused_memory += allocated;
  total_used_memory += allocated;

  return outP;
}

void kfree_page(void *ptr) {
  if (ptr == NULL) return;
  // kprintf("Free ptr %x ", p);
  u32 phys = PA((u32)ptr);
  get_lock(kmem_lock);
  BuddyBlockNew *free_me = buddy_new_from_phys(phys, &kernel_buddy_new);
  // kprintf("Buddy: ");
  // print_buddy_new(free_me);
  u32 freed_size = (PAGES_PER_BLOCK(free_me->order) * PAGE_SIZE);
  free_buddy_new(free_me, &kernel_buddy_new);
  total_kused_memory -= freed_size;
  total_used_memory -= freed_size;
  unlock(kmem_lock);
}

u32 calc_page_order(u32 byte_size) {
  u32 order = 0;
  u32 pages = byte_size / PAGE_SIZE + ((byte_size % PAGE_SIZE) > 0 ? 1 : 0);

  while (order <= MAX_ORDER) {
    if (pages <= (u32)PAGES_PER_BLOCK(order)) {
      // kprintf("Size %d -> Order: %d\n", size, order);
      return order;
    }
    order++;
  }
  return -1;
}

void init_memory_alloc() {
  total_kernel_pages = (boot_mmap.total_pages / KERNEL_RATIO);
  total_fast_pages = (boot_mmap.total_pages / FAST_MEM_RATIO);
  total_normal_pages =
      boot_mmap.total_pages - total_kernel_pages - total_fast_pages;
  kprintf("Total kernel pages %d\nTotal normal pages %d\nTotal fast pages %d\n",
          total_kernel_pages, total_normal_pages, total_fast_pages);
  kprintf("Alloc kernel buddy system\n");
  // buddy_init(&kernel_pages, &buddies, buddy, total_kernel_pages);
  init_buddy_new(total_kernel_pages, 0, &kernel_buddy_new);

  kprintf("Alloc fast buddy system\n");
  init_buddy_new(total_fast_pages, total_kernel_pages + 1, &fast_buddy_new);

  kprintf("Alloc normal buddy system\n");
  init_buddy_new(total_normal_pages, total_kernel_pages + total_fast_pages + 2,
                 &normal_buddy_new);

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