#include "kernel.h"
#include "files.h"

#include "../boot/multiboot.h"
#include "../cpu/gdt.h"
#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../drivers/timer.h"
#include "../cpu/types.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../lib/constants.h"
#include "../lib/strings.h"
#include "../lock/lock.h"
#include "../mem/mem.h"
#include "../mem/paging.h"
#include "../mem/slab.h"
#include "../proc/thread.h"
#include "../lib/list.h"
#include "../lib/shutdown.h"
#include "../lib/utils.h"
#include "../bin/binaries.h"

KMultiBoot2Info kMultiBootInfo;
bool kernel_init_ok = FALSE;
/*
  After running the content of meminit.asm we get called here
*/
void kernel_main(u32 magic, u32 addr) {
  clearScreen();
  setTextColor(WHITE);

  // Installing the Global Descriptor Table for the segments we will need
  kprintf("Installing GDT...\n");
  gdt_install();
  kPrintOKMessage("GTD Installed!\n");

  // Just setting a couple of pointers in our C variables, nothing special
  init_memory_ptrs();

  save_multiboot2_info(addr, magic);
  parse_multiboot_info(&kMultiBootInfo);

  kPrintOKMessage("Installing IRQs/ISRs...\n");
  isr_install();
  kPrintOKMessage("IRQs/ISRs installed!\n");

  kPrintOKMessage("Init Buddy...\n");
  init_memory_alloc();
  init_kernel_locks();

  init_slab_cache();

  kPrintOKMessage("Buddy initialized!\n");
  init_kernel_vma();

  kPrintOKMessage("Enabling kernel paging...");
  init_kernel_paging();
  //init_user_paging();

  kPrintOKMessage("Kernel paging enabled!");

  init_files();

  kPrintOKMessage("Enabling kernel procs...");
  init_kernel_proc();
  init_work_queue();
  kPrintOKMessage("Kernel procs enabled!");
  kPrintOKMessage("Kernel inizialized!");
  resetScreenColors();

  irq_install();

  srand(tick_count);
}
