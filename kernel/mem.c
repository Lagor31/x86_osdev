#include "../cpu/types.h"

#include "../drivers/screen.h"
#include "../kernel/kernel.h"

#include "mem.h"

/* Getting the _stack_address as set in assembly to denote the beginning of
 * freeily allocatable memory */
void meminit() {
  stack_pointer = (uint8_t *)_stack_address;
  kprintf("Stack Pointer: 0x%x\n", (uint32_t)stack_pointer);
  free_mem_addr = stack_pointer;
  kprintf("Free memory address: 0x%x\n", (uint32_t)free_mem_addr);
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
void *kmalloc(size_t size, uint8_t align) {
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