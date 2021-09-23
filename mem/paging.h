#ifndef PAGING_H
#define PAGING_H

#include "page.h"

#define PF_P 1
#define PF_W 2
#define PF_U 4
#define PF_R 8
#define PF_I 16

/*

31              4               0
+---+--  --+---+---+---+---+---+---+
|   Reserved   | I | R | U | W | P |
+---+--  --+---+---+---+---+---+---+
Length	Name	Description
P	1 bit	Present	When set, the page fault was caused by a page-protection violation. When not set, it was caused by a non-present page.
W	1 bit	Write	When set, the page fault was caused by a write access. When not set, it was caused by a read access.
U	1 bit	User	When set, the page fault was caused while CPL = 3. This does not necessarily mean that the page fault was a privilege violation.
R	1 bit	Reserved write	When set, one or more page directory entries contain reserved bits which are set to 1. This only applies when the PSE or PAE flags in CR4 are set to 1.
I	1 bit	Instruction Fetch	When set, the page fault was caused by an instruction fetch. This only applies when the No-Execute bit is supported and enabled.
In addition, it sets the value of the CR2 register to the virtual address which caused the Page Fault.ss

*/

extern void _jump_usermode();
extern void _jump_to_k_virtual_space();

extern u32 kernel_page_directory[1024];
//extern u32 user_page_directory[1024];

// Implemented in ASM
extern void _loadPageDirectory(unsigned int *);
extern void _enablePaging();

void init_kernel_paging();
void enableUserModePaging(void *processPhysAddress);
void pageFaultHandler(registers_t *regs);
void gpFaultHandler(registers_t *regs);
Pte *make_kernel_pte(uint32_t pdRow);
uint32_t *createPageTableUser(uint32_t pdRow);

void init_user_paging(u32 *pd);
bool is_kernel_addr(u32 addr);

#endif