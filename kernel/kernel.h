#ifndef KERNEL_H
#define KERNEL_H
#include "../proc/thread.h"

#define KERNEL_VIRTUAL_ADDRESS_BASE 0xC0000000
#define KERNEL_LOWEST_PDIR 768
#define KERNEL_RATIO 8
#define FAST_MEM_RATIO 20

#define PA(X) ((X) - (KERNEL_VIRTUAL_ADDRESS_BASE))
#define VA(X) ((X) + (KERNEL_VIRTUAL_ADDRESS_BASE))

#define ALLOC_NUM 50
#define ALLOC_SIZE 5
#define HOSTNAME "challenger"

extern KMultiBoot2Info kMultiBootInfo;
extern bool kernel_init_ok;
extern struct rfsHeader *krfsHeader;
extern struct fileTableEntry *kfileTable;

extern u32 kimage_start;
extern u32 kimage_end;
extern u32 kentry_point;
extern u32 _stack_address;
extern u32 _random(void);
extern void *userProcess;

extern u32 files_start;
extern u32 files_end;

void shutdown();
void printInitScreen();
void user_input(char *);
void showHelp();
void panic(char *);

#endif