#include "binaries.h"

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
    Thread *p = NULL;
    list_for_each(l, &running_queue) {
      p = list_entry(l, Thread, head);
      if (i++ == killMe) {
        kprintf("Stopping PID: %d\n", p->pid);
        stop_thread(p);
        break;
      }
    }
    resetScreenColors();
    // clearScreen();
    kprintf(">");
    _switch_to_thread((Thread *)do_schedule());

  } else if (!strcmp(input, "free")) {
    printFree();
  } else if (!strcmp(input, "head")) {
    Thread *p = NULL;
    p = create_kernel_thread(&top_bar, NULL, "head");
    p->nice = 0;
    wake_up_thread(p);
  } else if (!strcmp(input, "printtop")) {
    disable_int();
    printTop();
    enable_int();
  } else if (!strcmp(input, "top")) {
    Thread *p = NULL;
    p = create_kernel_thread(&top, NULL, "top");
    p->nice = 0;
    wake_up_thread(p);
  } else if (!strcmp(input, "ckp")) {
    Thread *p;
    for (int i = 0; i < ALLOC_NUM; ++i) {
      p = create_kernel_thread(&k_simple_proc, NULL, "k-extra");
      p->nice = rand() % 20;
      wake_up_thread(p);
    }
  } else if (!strcmp(input, "cup")) {
    Thread *p;
    for (int i = 0; i < ALLOC_NUM; ++i) {
      p = create_user_thread(&u_simple_proc, NULL, "u-extra");
      p->nice = rand() % 20;
      wake_up_thread(p);
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

void itaFlag() {
  clearScreen();

  setCursorPos(1, 0);
  u32 r = 0;
  u32 c = 0;
  for (r = 0; r < VGA_ROWS; ++r) {
    resetScreenColors();

    setTextColor(GREEN);
    setBackgroundColor(GREEN);
    for (c = 0; c < 26; ++c) kprintf(" ");

    resetScreenColors();

    setTextColor(WHITE);
    setBackgroundColor(WHITE);
    for (c = 0; c < 27; ++c) kprintf(" ");

    resetScreenColors();

    setTextColor(RED);
    setBackgroundColor(RED);
    for (c = 0; c < 26; ++c) kprintf(" ");
    kprintf("\n");
  }
  resetScreenColors();
  // clearScreen();
}
