#ifndef PAGING_H
#define PAGING_H

#include "page.h"



extern void _jump_usermode();
extern void _jump_to_k_virtual_space();

extern uint32_t kernel_page_directory[1024];
extern uint32_t user_page_directory[1024];
// Implemented in ASM
extern void _loadPageDirectory(unsigned int *);
extern void _enablePaging();

void init_kernel_paging();
void enableUserModePaging(void *processPhysAddress);
void pageFaultHandler(registers_t *regs);
void gpFaultHandler(registers_t *regs);
Pte *make_kernel_pte(uint32_t pdRow);
uint32_t *createPageTableUser(uint32_t pdRow);

#endif