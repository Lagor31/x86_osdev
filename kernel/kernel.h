#ifndef KERNEL_H
#define KERNEL_H


#define KERNEL_VIRTUAL_ADDRESS_BASE 0xC0000000

#define PA(X) ((X) - (0xC0000000))
#define VA(X) ((X) + (0xC0000000))

typedef void (*callme)(void);

extern struct kmultiboot2info *kMultiBootInfo;

extern struct rfsHeader *krfsHeader;
extern struct fileTableEntry *kfileTable;

extern uint32_t kimage_start;
extern uint32_t kimage_end;
extern uint32_t kentry_point;
extern uint32_t _stack_address;
extern uint32_t _random(void);
extern void *userProcess;

void shutdown();
void printInitScreen();
void user_input(char *);
void showHelp();

#endif