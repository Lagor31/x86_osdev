#include "kernel.h"

#include "../boot/multiboot.h"
#include "../cpu/gdt.h"
#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../cpu/timer.h"
#include "../cpu/types.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../libc/constants.h"
#include "../libc/strings.h"
#include "../lock/lock.h"
#include "../mem/mem.h"
#include "../mem/paging.h"
#include "../mem/slab.h"
#include "../proc/proc.h"
#include "../rfs/rfs.h"
#include "../utils/list.h"
#include "../utils/shutdown.h"
#include "../utils/utils.h"

KMultiBoot2Info *kMultiBootInfo;
struct rfsHeader *krfsHeader;
struct fileTableEntry *kfileTable;
u32 kernel_spin_lock = 0;

void k_simple_proc() {
  int c = 0;
  sleep_ms(rand() % 300);
  while (TRUE) {
    // kprintf("1) PID %d\n", current_proc->pid);
    // printProc(current_proc);

    // kprintf("Trying lock 0x%x PID: %d\n", &kernel_spin_lock,
    // current_proc->pid);
    /* while (_test_spin_lock(&kernel_spin_lock) == LOCK_LOCKED) {
      sleep_process(current_proc);
      _switch_to_task((Proc *)do_schedule());
    } */
    get_lock(screen_lock);

    // setCursorPos(current_proc->pid + 1, 50);
    // kprintf("Got lock 0x%x!!!\n", &kernel_spin_lock);
    kprintf("PID: %d N: %d (%d)\n", current_proc->pid, current_proc->nice, ++c);
    // printProcSimple(current_proc);
    // setCursorOffset(prevPos);
    // kprintf("Releasing lock 0x%x :(\n\n", &kernel_spin_lock);

    sleep_ms(1000);

    unlock(screen_lock);
    sleep_ms(10);

    // sleep_process(current_proc);
    //_switch_to_task((Proc *)do_schedule());
  }
}

void shell() {

  char *my_buf = normal_page_alloc(0);
  memset((byte *)my_buf, '\0', PAGE_SIZE);
  while (TRUE) {
    char read = read_stdin();
    if (read == '\n') {
      get_lock(screen_lock);
      kprintf("\n");
      user_input(my_buf);
      unlock(screen_lock);

      memset((byte *)my_buf, '\0', PAGE_SIZE);
    } else if (read == BACKSPACE) {
      if (strlen(my_buf) > 0) {
        backspace(my_buf);
        deleteLastChar();
      }
    } else {
      append(my_buf, read);
      get_lock(screen_lock);
      kprintf("%c", read);
      unlock(screen_lock);
    }
  }
}

void top_bar() {
  while (TRUE) {
    // kprintf("1) PID %d\n", current_proc->pid);
    // printProc(current_proc);

    u32 prevPos = getCursorOffset();

    setCursorPos(0, 0);
    setBackgroundColor(BLUE);
    setTextColor(YELLOW);

    int totFree = total_used_memory / 1024 / 1024;
    int tot = boot_mmap.total_pages * 4096 / 1024 / 1024;

    const char *title = " Uptime: %4ds             Used: %4d / %4d Mb"
                        "               ProcsRunning: %3d ";
    get_lock(screen_lock);

    kprintf(title, getUptime() / 1000, totFree, tot,
            list_length(&running_queue));
    unlock(screen_lock);

    setCursorOffset(prevPos);
    resetScreenColors();

    sleep_ms(100);
  }
}

void itaFlag() {
  clearScreen();

  setCursorPos(1, 0);
  u32 r = 0;
  u32 c = 0;
  for (r = 0; r < VGA_ROWS; ++r) {
    resetScreenColors();

    setTextColor(GREEN);
    setBackgroundColor(GREEN);
    for (c = 0; c < 26; ++c)
      kprintf(" ");

    resetScreenColors();

    setTextColor(WHITE);
    setBackgroundColor(WHITE);
    for (c = 0; c < 27; ++c)
      kprintf(" ");

    resetScreenColors();

    setTextColor(RED);
    setBackgroundColor(RED);
    for (c = 0; c < 26; ++c)
      kprintf(" ");
    kprintf("\n");
  }
  resetScreenColors();
  // clearScreen();
}
void k_simple_proc_no() {
  while (TRUE) {

    get_lock(mem_lock);
    char *string = normal_page_alloc(10);
    unlock(mem_lock);

    sleep_ms(400);
    string[0] = 'F';

    get_lock(mem_lock);
    kfreeNormal(string);
    unlock(mem_lock);
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

  init_kernel_locks();
  init_stdin();

  kPrintOKMessage("Kernel inizialized!");

  resetScreenColors();

  clearScreen();
  kprintf("\n>");

  Proc *p;
  /*  for (int i = 0; i < ALLOC_NUM; ++i) {
     p = create_kernel_proc(&k_simple_proc, NULL, "k-init");
     p->nice = 0;
     wake_up_process(p);
   } */

  p = create_kernel_proc(&top_bar, NULL, "head");
  p->nice = 20;
  wake_up_process(p);

  p = create_kernel_proc(&shell, NULL, "shell");
  p->nice = 0;
  wake_up_process(p);

  p = create_kernel_proc(&top, NULL, "top");
  p->nice = 10;
  wake_up_process(p);

  irq_install();
  // srand(tickCount);
  /*   clearScreen();
    kprintf("\n>"); */
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
  } else if (!strcmp(input, "flag")) {
    asm volatile("cli");
    itaFlag();
    asm volatile("sti");

  } else if (!strcmp(input, "stop")) {
    u32 numProc = list_length(&running_queue);
    u32 killMe = 0;

    killMe = rand() % numProc;

    List *l;
    u32 i = 0;
    Proc *p = NULL;
    list_for_each(l, &running_queue) {
      p = list_entry(l, Proc, head);
      if (i++ == killMe) {
        kprintf("Stopping PID: %d\n", p->pid);
        stop_process(p);
        break;
      }
    }
    resetScreenColors();
    // clearScreen();
    kprintf(">");
    _switch_to_task((Proc *)do_schedule());

  } else if (!strcmp(input, "free")) {
    printFree();
  } else if (!strcmp(input, "head")) {
    Proc *p = NULL;
    p = create_kernel_proc(&top_bar, NULL, "head");
    p->nice = 0;
    wake_up_process(p);
  } else if (!strcmp(input, "top")) {
    /* Proc *p = NULL;
    p = create_kernel_proc(&top, NULL, "top");
    p->nice = 0;
    wake_up_process(p);
 */
    printTop();
  } else if (!strcmp(input, "ckp")) {
    Proc *p;
    for (int i = 0; i < ALLOC_NUM; ++i) {
      p = create_kernel_proc(&k_simple_proc_no, NULL, "kthread");
      p->nice = 15;
      wake_up_process(p);
    }
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
  } else if (!strcmp(input, "shutdown")) {
    shutdown();
  } else if (!strcmp(input, "reboot")) {
    reboot();
  } else {
    kprintf("Command %s not found!", input);
  }
  kprintf("\n>");
}
