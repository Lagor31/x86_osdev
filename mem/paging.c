#include "../cpu/types.h"

#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../utils/list.h"
#include "../boot/multiboot.h"
#include "../drivers/screen.h"
#include "../libc/constants.h"
#include "../libc/functions.h"
#include "../utils/utils.h"
#include "../kernel/kernel.h"
#include "../mem/mem.h"

#include <elf.h>

#include "../cpu/gdt.h"

#include "../cpu/timer.h"

#include "paging.h"

uint32_t kernel_page_directory[1024] __attribute__((aligned(4096)));

uint32_t user_page_directory[1024] __attribute__((aligned(4096)));
uint32_t pdPhysical = 0;

uint32_t kVirtualNextInstr;

void gpFaultHandler(registers_t *regs) {
  //_loadPageDirectory((uint32_t *)PA((uint32_t)kernel_page_directory));

  setBackgroundColor(BLUE);
  setTextColor(RED);
  kprintf("GP Fault CS:EIP 0x%x:0x%x ErrNo: %d\n", regs->cs, regs->eip,
          regs->err_code);
  resetScreenColors();
  hlt();
}

void pageFaultHandler(registers_t *regs) {
  uint8_t userMode = FALSE;
  /*  if ((regs->cs & 0b11) == 3) userMode = TRUE; */

  /* if (userMode)
    _loadPageDirectory((uint32_t *)PA((uint32_t)kernel_page_directory));
 */
 /*  setBackgroundColor(BLUE);
  setTextColor(RED);
  kprintf("Page fault EIP 0x%x Code: %d\n", regs->eip, regs->err_code);
  kprintf("CR2 Value: 0x%x\n", getRegisterValue(CR2)); */

  uint32_t faultAddress = getRegisterValue(CR2);
  if (faultAddress < KERNEL_VIRTUAL_ADDRESS_BASE) {
    resetScreenColors();
    kPrintKOMessage("Not a kernel address!");
    hlt();
  }

  uint32_t pd_pos = faultAddress >> 22;
  uint32_t pte_pos = faultAddress >> 12 & 0x3FF;
  uint32_t pfn = PA(faultAddress) >> 12;
  if (kernel_page_directory[pd_pos] != 0) {
   /*  kprintf(
        "The 4Mb Page containing the address has already been allocated, "
        "checking PTE...\n"); */
    Pte *pte = VA(kernel_page_directory[pd_pos] & 0xFFFFF000);
    if (pte[pte_pos] == 0) {
      /* kprintf(
          "4KB page containing the address is not mapped.\nNeeds to be set to "
          "pfn %d\n",
          pfn); */
      setPfn(&pte[pte_pos], pfn);
      setPresent(&pte[pte_pos]);
      setReadWrite(&pte[pte_pos]);
    }
  } else {
/*     kprintf("The 4MB page was NOT allocated!\n");
 */    Pte *newPte = (Pte *)kernel_page_alloc(0);
    // memset((uint8_t *)newPte, 0, PAGE_SIZE);
    setPfn(&newPte[pte_pos], pfn);
    setPresent(&newPte[pte_pos]);
    setReadWrite(&newPte[pte_pos]);
    uint32_t pde_phys = PA((uint32_t)newPte);
    setReadWrite(&pde_phys);
    setPresent(&pde_phys);
    kernel_page_directory[pd_pos] = pde_phys;
  }
  /*  kernel_page_directory[lastAllocatedEntry] =
       kernel_page_directory[lastAllocatedEntry - 768]; */
  // Very simple page fault handling, the pages are already setup, we just
  // put the present bit to 1 because we have no disk to load pages from
  /* kprintf("Repairing page fault\nPD[%d] PTE[%d] PHY=%x\n", pd_pos, pte_pos,
          PA(faultAddress)); */
  _loadPageDirectory((uint32_t *)PA((uint32_t)&kernel_page_directory));
/*   kprintf("Repaired!\n");
 */  resetScreenColors();

  /*  if (userMode) {
     user_page_directory[0] &= 0xFFFFFFDF;
     user_page_directory[1] &= 0xFFFFFFDF;
     _loadPageDirectory((uint32_t *)PA(
         (uint32_t)&user_page_directory));  // REmove A(ccessed) bit
   } */
}

void enableUserModePaging(void *processVA) {
  uint32_t *userPageTable = (uint32_t *)boot_alloc(sizeof(uint32_t) * 1024, 1);
  userPageTable[0] = (PA((uint32_t)processVA) & 0xFFFFF000) | 7;
  user_page_directory[0] = PA((uint32_t)userPageTable) | 7;
  user_page_directory[1] =
      (((uint32_t)createPageTableUser(0)) - KERNEL_VIRTUAL_ADDRESS_BASE) | 7;

  // I map 4 pages of kernel into the user Virtual Address Space
  for (int i = 768; i < 768 + 4; ++i) {
    user_page_directory[i] = kernel_page_directory[i];
  }

  // Elf32_Ehdr *elfHeader = (Elf32_Ehdr *)processVA;

  /*   kprintf("ELF Entry point: 0x%x\n", elfHeader->e_entry);
    kprintf("Machine: %x Flags: %x Vers: %x\n", elfHeader->e_machine,
            elfHeader->e_flags, elfHeader->e_version); */

  tss.cs = 0x10;
  tss.esp0 = getRegisterValue(ESP);
  /* After every interrupt we need to send an EOI to the PICs
   * or they will not send another interrupt again */
  outb(0xA0, 0x20); /* slave */
  outb(0x20, 0x20); /* master */
  _loadPageDirectory((uint32_t *)PA((uint32_t)user_page_directory));
}

uint32_t *createPageTableUser(uint32_t pdRow) {
  uint32_t baseFrameNumber = pdRow * 1024;
  uint32_t *pt = boot_alloc(sizeof(uint32_t) * 1024, 1);
  for (uint32_t i = 0; i < 1024; ++i) {
    uint32_t curFrameNumber = (baseFrameNumber + i) << 12;
    pt[i] = curFrameNumber | 0x107;
  }
  return pt;
}

Pte *make_kernel_pte(uint32_t pdRow) {
  uint32_t baseFrameNumber = pdRow * 1024;
  Pte *pte = kernel_page_alloc(0);
  memset((uint8_t *)pte, 0, PAGE_SIZE);
  for (uint32_t i = 0; i < PT_SIZE; ++i) {
    // pte[i] = curFrameNumber;
    setPfn(&pte[i], baseFrameNumber + i);
    setPresent(&pte[i]);
    setReadWrite(&pte[i]);
  }
  return pte;
}

void init_kernel_paging() {
  kprintf("Setting up kernel paging...\n");
  int num_entries = (total_kernel_pages >> 10);
  kprintf("Tot: %d Num entried PD: %d\n", total_kernel_pages, num_entries);
  uint16_t i = 0;
  for (i = 0; i < PD_SIZE; i++) {
    // Mapping the higher half kernel
    // Mapping the first page because we have some low addresses
    // lying around from earlier stages of the kernel
    if (i == 0) {
      uint32_t ptePhys = PA((uint32_t)make_kernel_pte(i));
      setPresent(&ptePhys);
      setReadWrite(&ptePhys);
      kernel_page_directory[i] = ptePhys;
    } else if (i >= KERNEL_LOWEST_PDIR &&
               i < KERNEL_LOWEST_PDIR + num_entries) {
      uint32_t ptePhys = PA((uint32_t)make_kernel_pte(i - 768));
      setPresent(&ptePhys);
      setReadWrite(&ptePhys);
      kernel_page_directory[i] = ptePhys;
    }
  }

  // kprintf("Kernel paging subsystem size = %d bytes\n", free_mem_addr - s);
  kprintf("Total kernel paging system size: %d Kb\n",
          num_entries * 4096 / 1024);
  kprintf("Max kernel  size: %d Mb\n", num_entries * 4);
  pdPhysical = PA((uint32_t)kernel_page_directory);
  // kprintf("KPDAdr: 0x%x\nPhysical: 0x%x\n", kernel_page_directory,
  // pdPhysical);
  _loadPageDirectory((uint32_t *)pdPhysical);
}