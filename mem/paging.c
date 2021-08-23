#include "../cpu/types.h"

#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../lib/list.h"
#include "../boot/multiboot.h"
#include "../drivers/screen.h"
#include "../lib/constants.h"
#include "../lib/utils.h"
#include "../kernel/kernel.h"
#include "../mem/mem.h"
#include "../mem/vma.h"
#include "../proc/thread.h"
#include "../lib/utils.h"

#include "../cpu/gdt.h"

#include "../drivers/timer.h"

#include "paging.h"
#include "../proc/thread.h"
#include "../kernel/scheduler.h"

u32 kernel_page_directory[1024] __attribute__((aligned(4096)));
u32 user_page_directory[1024] __attribute__((aligned(4096)));
u32 pdPhysical = 0;

void gpFaultHandler(registers_t *regs) {
  //_loadPageDirectory((uint32_t *)PA((uint32_t)kernel_page_directory));

  setBackgroundColor(BLUE);
  setTextColor(RED);
  if (current_thread != NULL) printProcSimple(current_thread);

  kprintf("GP Fault CS:EIP 0x%x:0x%x ErrNo: %d Syscall: %d\n", regs->cs,
          regs->eip, regs->err_code, regs->ebx);
  resetScreenColors();
  hlt();
}

u32 calc_pfn(u32 addr, MemDesc *mem_desc) {
  VMArea *vma;
  List *l;
  list_for_each(l, &mem_desc->vm_areas) {
    vma = list_entry(l, VMArea, head);
    if (addr < vma->end && addr >= vma->start) {
      u32 virt_offset = addr - vma->start;
      u32 phys_addr = vma->phys_start + virt_offset;

      return phys_addr >> 12;
    }
  }
  return -1;
}

void pageFaultHandler(registers_t *regs) {
  UNUSED(regs);
  /* setBackgroundColor(BLUE);
  setTextColor(RED);
  kprintf("Page fault CS:EIP 0x%x:0x%x Code: %d\n", regs->cs, regs->eip,
          regs->err_code);
  kprintf("CR2 Value: 0x%x\n", getRegisterValue(CR2)); */

  u32 faultAddress = getRegisterValue(CR2);

  /* Handling user mode pagefault */
  if (current_thread != NULL && current_thread->ring0 != TRUE) {
    setBackgroundColor(RED);
    setTextColor(WHITE);
    kprintf("Page fault CS:EIP 0x%x:0x%x Code: %d\n", regs->cs, regs->eip,
            regs->err_code);
    kprintf("CR2 Value: 0x%x\n", getRegisterValue(CR2));
    resetScreenColors();

    if (!is_valid_va(faultAddress, current_thread)) {
      setBackgroundColor(RED);
      setTextColor(WHITE);
      kprintf("0x%x - Not a thread address!\n", faultAddress);
      kill_process(current_thread);
      reschedule();
      return;
    }

    u32 pd_pos = faultAddress >> 22;
    u32 pte_pos = faultAddress >> 12 & 0x3FF;
    u32 pfn = calc_pfn(faultAddress, current_thread->mem);

    u32 *user_pd = (u32 *)VA(current_thread->tcb.page_dir);
    if (isPresent((u32 *)user_pd[pd_pos])) {
      kprintf(
          "The 4Mb Page containing the address has already been allocated, "
          "checking PTE...\n");
      Pte *pte = (Pte *)VA(user_pd[pd_pos] & 0xFFFFF000);
      if (!isPresent(&pte[pte_pos])) {
        kprintf(
            "4KB page containing the address is not mapped.\nNeeds to be set   "
            "        to "
            "pfn %d\n",
            pfn);
        setPfn(&pte[pte_pos], pfn);
        setPresent(&pte[pte_pos]);
        setReadWrite(&pte[pte_pos]);
        setUsermode(&pte[pte_pos]);
      }
    } else {
      kprintf("The 4MB page was NOT allocated!\n");

      Pte *newPte = (Pte *)kernel_page_alloc(0);
      memset((byte *)newPte, 0, PAGE_SIZE);
      setPfn(&newPte[pte_pos], pfn);
      setPresent(&newPte[pte_pos]);
      setReadWrite(&newPte[pte_pos]);
      setUsermode(&newPte[pte_pos]);

      u32 pde_phys = PA((u32)newPte);
      setReadWrite(&pde_phys);
      setPresent(&pde_phys);
      setUsermode(&pde_phys);
      user_pd[pd_pos] = pde_phys;
      // kernel_page_directory[pd_pos] = pde_phys;
    }

    /* kill_process(current_thread);
    reschedule(); */
    return;
  } else {
    /* Handling kernel mode page fault */
    u32 pd_pos = faultAddress >> 22;
    u32 pte_pos = faultAddress >> 12 & 0x3FF;
    u32 pfn = PA(faultAddress) >> 12;
    if (isPresent(&kernel_page_directory[pd_pos])) {
      /*  kprintf(
          "The 4Mb Page containing the address has already been allocated, "
          "checking PTE...\n");  */
      Pte *pte = (Pte *)VA(kernel_page_directory[pd_pos] & 0xFFFFF000);
      if (!isPresent(&pte[pte_pos])) {
        /*   kprintf(
              "4KB page containing the address is not mapped.\nNeeds to be set
           to " "pfn %d\n", pfn); */
        setPfn(&pte[pte_pos], pfn);
        setPresent(&pte[pte_pos]);
        setReadWrite(&pte[pte_pos]);
      }
    } else {
      /*     kprintf("The 4MB page was NOT allocated!\n");
       */
      Pte *newPte = (Pte *)kernel_page_alloc(0);
      memset((byte *)newPte, 0, PAGE_SIZE);
      setPfn(&newPte[pte_pos], pfn);
      setPresent(&newPte[pte_pos]);
      setReadWrite(&newPte[pte_pos]);

      u32 pde_phys = PA((u32)newPte);
      setReadWrite(&pde_phys);
      setPresent(&pde_phys);

      kernel_page_directory[pd_pos] = pde_phys;
    }
  }
  resetScreenColors();
}

Pte *make_kernel_pte(uint32_t pdRow) {
  uint32_t baseFrameNumber = pdRow * 1024;
  Pte *pte = kernel_page_alloc(0);
  memset((byte *)pte, 0, PAGE_SIZE);
  for (u32 i = 0; i < PT_SIZE; ++i) {
    // pte[i] = curFrameNumber;
    setPfn(&pte[i], baseFrameNumber + i);
    setPresent(&pte[i]);
    setReadWrite(&pte[i]);
  }
  return pte;
}

Pte *make_user_pte(uint32_t pdRow) {
  uint32_t baseFrameNumber = pdRow * 1024;
  Pte *pte = kernel_page_alloc(0);
  memset((byte *)pte, 0, PAGE_SIZE);
  for (u32 i = 0; i < PT_SIZE; ++i) {
    // pte[i] = curFrameNumber;
    setPfn(&pte[i], baseFrameNumber + i);
    setPresent(&pte[i]);
    setReadWrite(&pte[i]);
    setUsermode(&pte[i]);
  }
  return pte;
}

void init_test_user_paging() {
  kprintf("Setting up test user paging...\n");
  u16 i = 0;
  //int num_entries = (total_kernel_pages >> 10);

  for (i = 0; i < PD_SIZE; i++) {
    // Mapping the higher half kernel
    // Mapping the first page because we have some low addresses
    // lying around from earlier stages of the kernel

    u32 ptePhys;
    if (i >= KERNEL_LOWEST_PDIR)
      ptePhys = PA((u32)make_kernel_pte(i - KERNEL_LOWEST_PDIR));
    else if (i == 0)
      ptePhys = PA((u32)make_kernel_pte(i));
    else {
      user_page_directory[i] = 0;
      continue;
    }

    setPresent(&ptePhys);
    setReadWrite(&ptePhys);

    user_page_directory[i] = ptePhys;

    /* if (i == 0 ||
        (i >= KERNEL_LOWEST_PDIR && i < KERNEL_LOWEST_PDIR + num_entries))
      user_page_directory[i] = kernel_page_directory[i];
 */
    // setUsermode(&ptePhys);
  }
}

void init_kernel_paging() {
  kprintf("Setting up kernel paging...\n");
  int num_entries = (total_kernel_pages >> 10);
  u16 i = 0;
  for (i = 0; i < PD_SIZE; i++) {
    // Mapping the higher half kernel
    // Mapping the first page because we have some low addresses
    // lying around from earlier stages of the kernel
    if (i == 0) {
      u32 ptePhys = PA((u32)make_kernel_pte(i));
      setPresent(&ptePhys);
      setReadWrite(&ptePhys);
      kernel_page_directory[i] = ptePhys;
    } else if (i >= KERNEL_LOWEST_PDIR &&
               i < KERNEL_LOWEST_PDIR + num_entries) {
      u32 ptePhys = PA((u32)make_kernel_pte(i - KERNEL_LOWEST_PDIR));
      setPresent(&ptePhys);
      setReadWrite(&ptePhys);
      kernel_page_directory[i] = ptePhys;
    }
  }

  // kprintf("Kernel paging subsystem size = %d bytes\n", free_mem_addr - s);
  kprintf("Total kernel paging system size: %d Kb\n",
          num_entries * 4096 / 1024);
  kprintf("Max kernel  size: %d Mb\n", num_entries * 4);
  pdPhysical = PA((u32)kernel_page_directory);
  // kprintf("KPDAdr: 0x%x\nPhysical: 0x%x\n", kernel_page_directory,
  // pdPhysical);
  _loadPageDirectory((u32 *)pdPhysical);
}