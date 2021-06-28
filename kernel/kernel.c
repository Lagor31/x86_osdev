#include "../cpu/types.h"

#include "../utils/utils.h"

#include "../boot/multiboot.h"

#include "../cpu/gdt.h"
#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../libc/constants.h"
#include "../libc/strings.h"
#include "../mem/mem.h"
#include "../mem/paging.h"
#include "../mem/slab.h"
#include "../proc/proc.h"
#include "../rfs/rfs.h"
#include "../utils/list.h"
#include "../utils/shutdown.h"

#include "../cpu/timer.h"

#include "kernel.h"

KMultiBoot2Info *kMultiBootInfo;
struct rfsHeader *krfsHeader;
struct fileTableEntry *kfileTable;

u8 firstTime = 1;
void **kfrees;
void **nfrees;
Proc *ping[ALLOC_NUM];
Proc *topTask;

void k_simple_proc() {
  int c = 0;
  while (TRUE) {
    // kprintf("1) PID %d\n", current_proc->pid);
    // printProc(current_proc);
    asm("cli");

    char *string = normal_page_alloc(10);
    char *s = "Hey!";
    memcopy((byte *)s, (byte *)string, strlen(s));

    u32 prevPos = getCursorOffset();
    setCursorPos(current_proc->pid + 1, 40);
    kprintf("PID: %d P: %d (%d) %s", current_proc->pid, current_proc->p, ++c,
            string);
    // printProcSimple(current_proc);
    setCursorOffset(prevPos);
    kfreeNormal(string);

    /*     sleep_process(current_proc);
        _switch_to_task((Proc *)do_schedule());
     */
    asm("sti");
    //syncWait(1000);

    /*  asm("cli");
     stop_process(current_proc);
     _switch_to_task((Proc *)do_schedule());
     asm("sti"); */

    //__asm__ __volatile__("hlt");
  }
}

void top_bar() {
  while (TRUE) {
    // kprintf("1) PID %d\n", current_proc->pid);
    // printProc(current_proc);

    asm("cli");

    u32 prevPos = getCursorOffset();
    setCursorPos(0, 0);
    setBackgroundColor(WHITE);
    setTextColor(BLUE);

    int totFree = total_used_memory / 1024 / 1024;
    int tot = boot_mmap.total_pages * 4096 / 1024 / 1024;

    const char *title = " Uptime: %ds           Used: %d / %d Mb"
                        "                     ProcsRunning: %d ";
    kprintf(title, getUptime() / 1000, totFree, tot,
            list_length(&running_queue));
    setCursorOffset(prevPos);
    resetScreenColors();
    asm("sti");
    syncWait(50);
    //__asm__ __volatile__("hlt");
  }
}

void k_simple_proc_no() {
  while (TRUE) {
    // kprintf("1) PID %d\n", current_proc->pid);
    // printProc(current_proc);
    asm("cli");
    char *string = normal_page_alloc(10);
    string[0] = 'F';
    kfreeNormal(string);
    asm("sti");

    syncWait(100);
    // u8 i = rand() % ALLOC_NUM;
    /* wake_up_process(ping[i]);
    sleep_process(current_proc);
    _switch_to_task(ping[i]); */
    /*  wake_up_process(ping[1]);
     sleep_process(ping[0]);


     printProc(ping[1]); */
    // load_current_proc(ping[0]);
    //_switch_to_task(ping[1]);
    // sleep_process(current_proc);
    // do_schedule();
    // kprintf("Now scheduling PID: %d\n", current_proc->pid);
    //_switch_to_task(current_proc);
    __asm__ __volatile__("hlt");
  }
}

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

  kfrees = boot_alloc(ALLOC_NUM * sizeof(void *), 1);
  nfrees = boot_alloc(ALLOC_NUM * sizeof(void *), 1);

  kPrintOKMessage("Installing IRQs/ISRs...\n");
  isr_install();
  kPrintOKMessage("IRQs/ISRs installed!\n");

  kPrintOKMessage("Init Buddy...\n");
  memory_alloc_init();
  kPrintOKMessage("Buddy initialized!\n");

  kPrintOKMessage("Enabling kernel paging...");
  init_kernel_paging();
  kPrintOKMessage("Kernel paging enabled!");

  kMemCacheInit();

  kPrintOKMessage("Enabling kernel procs...");
  init_kernel_proc();
  kPrintOKMessage("Kernel procs enabled!");

  kPrintOKMessage("Kernel inizialized!");

  resetScreenColors();

  clearScreen();
  kprintf("\n>");

  Proc *p;
  for (int i = 0; i < ALLOC_NUM; ++i) {
    p = create_kernel_proc(&k_simple_proc, NULL, "initp-aaaa");
    p->p = rand() % 20;
    ping[i] = p;
    wake_up_process(p);
  }

  p = create_kernel_proc(&top_bar, NULL, "topb-aaaa");
  p->p = 0;
  wake_up_process(p);

  p = create_kernel_proc(&top, NULL, "proc-aaaa");
  p->p = 0;
  wake_up_process(p);

  irq_install();
  srand(tickCount);

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
  } else if (!strcmp(input, "top")) {
    top();
  } else if (!strcmp(input, "ckp")) {
    Proc *p;
    for (int i = 0; i < ALLOC_NUM; ++i) {
      p = create_kernel_proc(&k_simple_proc_no, NULL, "kproc-aaaa");
      p->p = rand() % 20;
      wake_up_process(p);
    }
  } else if (!strcmp(input, "cup")) {
    Proc *p = NULL;

    for (int i = 0; i < ALLOC_NUM; ++i) {
      p = create_user_proc(&u_simple_proc, NULL, "uproc-aaaa");
      p->p = rand() % 20;
      wake_up_process(p);
    }

    // do_schedule();

    //_switch_to_task(current_proc);

  } else if (!strcmp(input, "bootinfo")) {
    printMultibootInfo((KMultiBoot2Info *)kMultiBootInfo, 0);
  } else if (!strcmp(input, "mmap")) {
    printMultibootInfo((KMultiBoot2Info *)kMultiBootInfo, 1);
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
      kfrees[i] = kernel_page_alloc(ALLOC_SIZE);
      u8 *a = kfrees[i];
      if (a == NULL)
        break;

      uint32_t pd_pos = (uint32_t)a >> 22;
      uint32_t pte_pos = (uint32_t)a >> 12 & 0x3FF;
      Pte *pte = (Pte *)VA((uint32_t)kernel_page_directory[pd_pos]);
      kprintf("Addr = 0x%x PD[%d], PTE[%d] = Phys->0x%x\n", a, pd_pos, pte_pos,
              ((pte[pte_pos] >> 20) * PAGE_SIZE));
      *a = 'F';
    }
  } else if (!strcmp(input, "nalloc")) {
    if (firstTime) {
      firstTime = 0;
      for (int i = 0; i < ALLOC_NUM; ++i) {
        nfrees[i] = normal_page_alloc(ALLOC_SIZE);
        u8 *a = nfrees[i];
        if ((u32)a < KERNEL_VIRTUAL_ADDRESS_BASE) {
          kprintf("Wrapped around?\n");

          break;
        }
        /*  uint32_t pd_pos = (uint32_t)a >> 22;
         uint32_t pte_pos = (uint32_t)a >> 12 & 0x3FF;
         Pte *pte = (Pte *)VA((uint32_t)kernel_page_directory[pd_pos] >> 12);
         kprintf("Addr = 0x%x PD[%d], PTE[%d] = PFN->0x%x\n", a, pd_pos,
         pte_pos, (pte[pte_pos] >> 12)); */

        *(a) = 'F';
        *(a + 1) = 0xD;
      }
      kPrintOKMessage("Done!");
    } else {
      for (int i = 0; i < ALLOC_NUM; ++i) {
        nfrees[i] = normal_page_alloc(ALLOC_SIZE);
        u8 *a = nfrees[i];
        if ((u32)a < KERNEL_VIRTUAL_ADDRESS_BASE) {
          kprintf("Wrapped around?\n");
          return;
        }
        /*   uint32_t pd_pos = (uint32_t)a >> 22;
          uint32_t pte_pos = (uint32_t)a >> 12 & 0x3FF;
          Pte *pte = (Pte *)VA((uint32_t)kernel_page_directory[pd_pos] >> 12);
          kprintf("Addr = 0x%x PD[%d], PTE[%d] = PFN->0x%x\n", a, pd_pos,
          pte_pos, (pte[pte_pos] >> 12)); */
        *(a) = 'X';
        *(a + 1) = 0xE;
      }
    }

  } else if (!strcmp(input, "kfree")) {
    for (int i = 0; i < ALLOC_NUM; ++i) {
      kfree(kfrees[i]);
    }
  } else if (!strcmp(input, "nfree")) {
    for (int i = 0; i < ALLOC_NUM; ++i) {
      kfreeNormal(nfrees[i]);
    }
  } else if (!strcmp(input, "shutdown")) {
    shutdown();
  } else if (!strcmp(input, "reboot")) {
    reboot();
  } else {
    kprintf("Command %s not found!", input);
  }
  kprintf("\n>");
}
