#ifndef KERNEL_H
#define KERNEL_H

#include "../cpu/types.h"


#define KERNEL_VIRTUAL_ADDRESS_BASE 0xC0000000
#define KERNEL_NORMAL_MEMORY_BASE 0xD0000000
#define KERNEL_LOWEST_PDIR 768
#define KERNEL_RATIO 8
#define PA(X) ((X) - (KERNEL_VIRTUAL_ADDRESS_BASE))
#define VA(X) ((X) + (KERNEL_VIRTUAL_ADDRESS_BASE))

#define ALLOC_NUM 2
#define ALLOC_SIZE 0
#define HOSTNAME "computer"

extern struct kmultiboot2info *kMultiBootInfo;

extern struct rfsHeader *krfsHeader;
extern struct fileTableEntry *kfileTable;

extern u32 kimage_start;
extern u32 kimage_end;
extern u32 kentry_point;
extern u32 _stack_address;
extern u32 _random(void);
extern void *userProcess;

void shutdown();
void printInitScreen();
void user_input(char *);
void showHelp();

#endif