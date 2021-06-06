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

// 5 pages * 4MB = 20MB allocated
#define PAGE_DIR_ENTRIES_NUM 1024
#define PAGE_DIR_LINES_ALLOCATED 4
uint32_t kernel_page_directory[1024] __attribute__((aligned(4096)));
uint32_t user_page_directory[1024] __attribute__((aligned(4096)));
uint32_t pdPhysical = 0;

uint32_t kVirtualNextInstr;
uint16_t lastAllocatedEntry = 0;

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
  setBackgroundColor(BLUE);
  setTextColor(RED);
  kprintf("Page fault EIP 0x%x Code: %d\n", regs->eip, regs->err_code);
  kprintf("CR2 Value: 0x%x\n", getRegisterValue(CR2));

  /*  kernel_page_directory[lastAllocatedEntry] =
       kernel_page_directory[lastAllocatedEntry - 768]; */
  // Very simple page fault handling, the pages are already setup, we just
  // put the present bit to 1 because we have no disk to load pages from
  lastAllocatedEntry++;
  resetScreenColors();
  hlt();
  if (userMode) {
    user_page_directory[0] &= 0xFFFFFFDF;
    user_page_directory[1] &= 0xFFFFFFDF;
    _loadPageDirectory((uint32_t *)PA(
        (uint32_t)&user_page_directory));  // REmove A(ccessed) bit
  }
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

uint32_t *createPageTable(uint32_t pdRow) {
  uint32_t baseFrameNumber = pdRow * 1024;
  // uint32_t *pt = boot_alloc(sizeof(uint32_t) * 1024, 1);
  uint32_t *pt = kmalloc(0);
  for (uint32_t i = 0; i < 1024; ++i) {
    uint32_t curFrameNumber = (baseFrameNumber + i) << 12;
    pt[i] = curFrameNumber | 3;
  }
  return pt;
}
/* Old */
void init_kernel_paging() {
  kprintf("Setting up kernel paging...\n");
  int num_entries = (total_kernel_pages >> 10);
  kprintf("Tot: %d Num entried PD: %d\n", total_kernel_pages, num_entries);
  uint16_t i = 0;
  for (i = 0; i < 1024; i++) {
    // Mapping the higher half kernel
    if (i < num_entries) {
      // kprintf("KPDG[%d] ", i);
      kernel_page_directory[i] =
          (((uint32_t)createPageTable(i)) - KERNEL_VIRTUAL_ADDRESS_BASE) | 3;
    } else if (i >= 768 && i < 768 + num_entries) {
      kernel_page_directory[i] =
          (((uint32_t)createPageTable(i - 768)) - KERNEL_VIRTUAL_ADDRESS_BASE) |
          3;
      // kprintf("KPDG[%d] ", i);
    }
  }

  // kprintf("Kernel paging subsystem size = %d bytes\n", free_mem_addr - s);
  kprintf("Total kernel paging system size: %d Kb\n",
          num_entries * 4096 / 1024);
  kprintf("Max kernel  size: %d Mb\n", num_entries * 4);
  pdPhysical = (uint32_t)kernel_page_directory - KERNEL_VIRTUAL_ADDRESS_BASE;
  lastAllocatedEntry = i;
  // kprintf("KPDAdr: 0x%x\nPhysical: 0x%x\n", kernel_page_directory,
  // pdPhysical);
  _loadPageDirectory((uint32_t *)pdPhysical);
}