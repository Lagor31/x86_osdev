#ifndef MEM_H
#define MEM_H

#define STACK_SIZE 0x1000
    
extern uint8_t *free_mem_addr; // Reppresents the first byte that we can freeily allocate
extern uint8_t *stack_pointer; // Top of the kernel stack

void meminit();
void memcopy(uint8_t *source, uint8_t *dest, size_t nbytes);
void *kmalloc(size_t size, uint8_t align);
void memset(uint8_t *dest, uint8_t val, uint32_t len);

#endif