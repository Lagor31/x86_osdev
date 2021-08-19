#include "binaries.h"
#include "../lib/list.h"
#include "../kernel/files.h"
#include "../drivers/cmos.h"
void shell() {
  setTextColor(LIGHTGREEN);
  setBackgroundColor(BLACK);
  kprintf("\n%s@%s # ", current_thread->owner->username, HOSTNAME);
  resetScreenColors();
  char *my_buf = normal_page_alloc(0);
  memset((byte *)my_buf, '\0', PAGE_SIZE);
  while (TRUE) {
    char read = read_stdin();
    if (read == '\n') {
      user_input(my_buf);
      setTextColor(LIGHTGREEN);
      setBackgroundColor(BLACK);
      kprintf("%s@%s # ", current_thread->owner->username, HOSTNAME);
      resetScreenColors();
      memset((byte *)my_buf, '\0', PAGE_SIZE);
    } else if (read == BACKSPACE) {
      if (strlen(my_buf) > 0) {
        backspace(my_buf);
        deleteLastChar();
      }
    } else {
      append(my_buf, read);
      kprintf("%c", read);
    }
  }
}

void user_input(char *input) {
  kprintf("\n");
  if (strcmp(input, "help") == 0)
    printHelp();
  else if (!strcmp(input, "clear")) {
    clearScreen();
  } else if (!strcmp(input, "exit")) {
    clearScreen();
    sys_exit(0);
  } else if (!strcmp(input, "flag")) {
    asm volatile("cli");
    itaFlag();
    asm volatile("sti");
  } else if (!strcmp(input, "free")) {
    printFree();
  } else if (!strcmp(input, "date")) {
    print_date();
  } else if (!strcmp(input, "lsof")) {
    List *l;
    list_for_each(l, &file_descriptors) {
      FD *f = (FD *)list_entry(l, FD, kfdq);
      kprintf("%s - %d Size: %d Avail: %d\n", f->name, f->fd, f->size,
              f->available);
    }

  } else if (!strcmp(input, "top")) {
    Thread *p = NULL;
    p = create_kernel_thread(&top, NULL, "top");
    p->nice = 0;
    wake_up_thread(p);
    sys_wait4(p->pid);
    clearScreen();
  } else if (!strcmp(input, "ps")) {
    Thread *p = NULL;
    p = create_kernel_thread(&ps, NULL, "ps");
    p->nice = 0;
    wake_up_thread(p);
    sys_wait4(p->pid);
  } else if (!strcmp(input, "ckp")) {
    Thread *p;
    for (int i = 0; i < ALLOC_NUM; ++i) {
      p = create_kernel_thread(&k_simple_proc, NULL, "k-extra");
      p->nice = 10;
      wake_up_thread(p);
    }
  } else if (!strcmp(input, "cup")) {
    Thread *p;
    for (int i = 0; i < ALLOC_NUM; ++i) {
      p = create_user_thread(&u_simple_proc, NULL, "u-extra");
      p->nice = 10;
      wake_up_thread(p);
    }
  } else if (!strcmp(input, "boot-info")) {
    printMultibootInfo(&kMultiBootInfo, 0);
  } else if (!strcmp(input, "boot-mmap")) {
    printMultibootInfo(&kMultiBootInfo, 1);
  } else if (!strcmp(input, "gdt")) {
    printGdt();
  } else if (!strcmp(input, "uptime")) {
    printUptime();
  } else if (!strcmp(input, "mods")) {
    printModuleInfo(getModule(&kMultiBootInfo));
  } else if (!strcmp(input, "shutdown")) {
    shutdown();
  } else if (!strcmp(input, "reboot")) {
    reboot();
  } else if (!strcmp(input, "id")) {
    kprintf("user: %s uid: %d gid: %d\n", current_thread->owner->username,
            current_thread->owner->uid, current_thread->owner->gid);
  } else {
    kprintf("Command '%s' not found!\n", input);
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
