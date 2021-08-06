#include "kernel.h"

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

KMultiBoot2Info *kMultiBootInfo;

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
  // kprintf("Kernel memory initialization...\n");
  init_memory_ptrs();

  saveMultibootInfo(addr, magic);
  parse_multiboot_info((KMultiBoot2Info *)kMultiBootInfo);

  kPrintOKMessage("Installing IRQs/ISRs...\n");
  isr_install();
  kPrintOKMessage("IRQs/ISRs installed!\n");

  kPrintOKMessage("Init Buddy...\n");
  memory_alloc_init();
  kPrintOKMessage("Buddy initialized!\n");

  kPrintOKMessage("Enabling kernel paging...");
  init_kernel_paging();
  init_test_user_paging();

  kPrintOKMessage("Kernel paging enabled!");

  kPrintOKMessage("Enabling chaching...");
  kMemCacheInit();
  kPrintOKMessage("Kernel caching enabled!");

  kPrintOKMessage("Enabling kernel procs...");
  init_kernel_proc();
  kPrintOKMessage("Kernel procs enabled!");

  init_kernel_locks();
  init_stdin();
  init_work_queue();

  kPrintOKMessage("Kernel inizialized!");

  resetScreenColors();

  // clearScreen();
  kprintf("\n>");

  Thread *p;

  for (int i = 0; i < ALLOC_NUM; ++i) {
    p = create_user_thread(&u_simple_proc, NULL, "uproc");
    p->nice = 10;
    wake_up_thread(p);
  }
  
  for (int i = 0; i < ALLOC_NUM; ++i) {
    p = create_kernel_thread(&k_simple_proc, NULL, "k-init");
    p->nice = 10;
    wake_up_thread(p);
  }

  p = create_kernel_thread(&top_bar, NULL, "head");
  p->nice = 0;
  wake_up_thread(p);

  p = create_kernel_thread(&shell, NULL, "shell");
  p->nice = 0;
  wake_up_thread(p);

  irq_install();
  srand(tick_count);
  clearScreen();
  kprintf("\n>");

  while (TRUE) hlt();
}
