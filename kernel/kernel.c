#include "../cpu/types.h"

#include "../boot/multiboot.h"

#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../libc/constants.h"
#include "../libc/strings.h"
#include "../utils/list.h"
#include "../utils/shutdown.h"
#include "../utils/utils.h"
#include "../cpu/gdt.h"
#include "../mem/mem.h"
#include "../mem/slab.h"

#include "../mem/paging.h"
#include "../mem/slab.h"
#include "../rfs/rfs.h"

#include "../cpu/timer.h"

#include "kernel.h"

#define ALLOC_NUM 10

struct kmultiboot2info *kMultiBootInfo;
struct rfsHeader *krfsHeader;
struct fileTableEntry *kfileTable;

// void (*userProcess)(void);
void *userProcess;

void **kfrees;
void **nfrees;

void runProcess() {
  enableUserModePaging(userProcess);
  _jump_usermode(0x110);
}

/*
  After running the content of meminit.asm we get called here
*/
void kernel_main(uint32_t magic, uint32_t addr) {
  clearScreen();
  setTextColor(WHITE);

  // Installing the Global Descriptor Table for the segments we will need
  kprintf("Installing GDT...\n");
  gdt_install();
  kPrintOKMessage("GTD Installed!\n");

  // Just setting a couple of pointers in our C variables, nothing special
  // kprintf("Kernel memory initialization...\n");
  init_memory_subsystem();

  saveMultibootInfo(addr, magic);
  parse_multiboot_info((struct kmultiboot2info *)kMultiBootInfo);

  kfrees = boot_alloc(ALLOC_NUM * sizeof(void *), 1);
  nfrees = boot_alloc(ALLOC_NUM * sizeof(void *), 1);

  kPrintOKMessage("Installing IRQs/ISRs...\n");
  isr_install();
  irq_install();
  kPrintOKMessage("IRQs/ISRs installed!\n");

  kPrintOKMessage("Init Buddy...\n");
  memory_alloc_init();
  kPrintOKMessage("Buddy initialized!\n");

  kPrintOKMessage("Enabling kernel paging...");
  init_kernel_paging();
  kPrintOKMessage("Kernel paging enabled!");

  // kMemCacheInit();

  kPrintOKMessage("Kernel inizialized!");
  // kPrintOKMessage("Kernel paging enabled");

  // We setup a Page mapping allowing the kernel to transparently use Virtual
  // Addresses starting from 0xC0000000
  // kprintf("Enabling kernel paging...\n");
  // Installing the interrupt/exception handlers
  // kprintf("Installing Interrupts Handlers...\n");

  // kPrintOKMessage("Interrupts Handlers installed...");

  // loadUserProcess(getModule(kMultiBootInfo));

  resetScreenColors();

  // Cute but utterly fake loading bar, used just to set the tick count to a
  // meaningful value for srand

 /*  setBackgroundColor(LIGHTGREEN);
  fakeSysLoadingBar(100);
  resetScreenColors(); */

  // srand(tickCount);
  // kPrintOKMessage("Kernel loaded successfully!");

  // syncWait(1000);

  /* clearScreen();
  printInitScreen(); */

  // Print the prompt and we're done, from now on we hlt the cpu until an
  // external interrupts gives control back to our OS
  // setCursorPos(2, 0);
  kprintf("\n>");
  // runProcess();
  hlt();
}

/*
  Sort of a kernel level shell that interpets a few of the commands a user can
  give to the terminal
*/
void user_input(char *input) {
  if (strcmp(input, "help") == 0)
    printHelp();
  else if (!strcmp(input, "clear")) {
    clearScreen();
    setCursorPos(2, 0);
  } else if (!strcmp(input, "free")) {
    printFree();
  } else if (!strcmp(input, "bootinfo")) {
    printMultibootInfo((struct kmultiboot2info *)kMultiBootInfo, 0);
  } else if (!strcmp(input, "mmap")) {
    printMultibootInfo((struct kmultiboot2info *)kMultiBootInfo, 1);
  } else if (!strcmp(input, "meminfo")) {
    printKernelMemInfo();
  } else if (!strcmp(input, "init")) {
    clearScreen();
    printInitScreen();
  } else if (!strcmp(input, "rfs")) {
    printRFSInfo();
  } else if (!strcmp(input, "gdt")) {
    printGdt();
  } else if (!strcmp(input, "uptime")) {
    printUptime();
  } else if (!strcmp(input, "mods")) {
    printModuleInfo(getModule(kMultiBootInfo));
  } else if (!strcmp(input, "kalloc")) {
    for (int i = 0; i < ALLOC_NUM; ++i) {
      kfrees[i] = kmalloc(10);
      uint8_t **a = kfrees[i];
      kprintf("Addr = 0x%x\n", a);
      //*a = 1;
    }
    /* for (int i = 0; i < ALLOC_NUM; ++i) {
      kfree(frees[i]);
    } */
  } else if (!strcmp(input, "nalloc")) {
    for (int i = 0; i < ALLOC_NUM; ++i) {
      nfrees[i] = normalAlloc(10);
      uint8_t **a = nfrees[i];
      kprintf("Addr = 0x%x\n", a);
      //*a = 1;
    }
    /* for (int i = 0; i < ALLOC_NUM; ++i) {
      kfree(frees[i]);
    } */
  } else if (!strcmp(input, "kfree")) {
    for (int i = 0; i < ALLOC_NUM; ++i) {
      kfree(kfrees[i]);
    }
  } else if (!strcmp(input, "nfree")) {
    for (int i = 0; i < ALLOC_NUM; ++i) {
      kfreeNormal(nfrees[i]);
    }
  } else if (!strcmp(input, "run")) {
    kprintf("\n>");
    input[0] = '\0';
    runProcess();
  } else if (!strcmp(input, "shutdown")) {
    shutdown();
  } else if (!strcmp(input, "reboot")) {
    reboot();
  } else {
    kprintf("Command %s not found!", input);
  }
  kprintf("\n>");
}
