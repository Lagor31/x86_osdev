#include "binaries.h"
#include "../lib/list.h"
#include "../kernel/files.h"
#include "../drivers/cmos.h"
#include "../drivers/pci.h"
#include "../kernel/kernel.h"
#include "../kernel/elf.h"

void print_prompt() {
  setTextColor(LIGHTGREEN);
  setBackgroundColor(BLACK);
  kprintf("\n%s@%s # ", current_thread->owner->username, HOSTNAME);
  resetScreenColors();
}
void shell() {
  char prev_read = '\0';

  resetScreenColors();

  print_prompt();
  char *my_buf = kalloc(0);
  memset((byte *)my_buf, '\0', PAGE_SIZE);
  while (TRUE) {
    char read = read_stdin();

    if (read == 'l' && prev_read == CTRL) {
      clearScreen();
      print_prompt();
      prev_read = read;
      continue;
    }

    prev_read = read;

    if (read == '\n') {
      user_input(my_buf);
      print_prompt();
      memset((byte *)my_buf, '\0', PAGE_SIZE);
    } else if (read == BACKSPACE) {
      if (strlen(my_buf) > 0) {
        backspace(my_buf);
        deleteLastChar();
      }
    } else if (read == CTRL) {
      continue;
    } else {
      append(my_buf, read);
      kprintf("%c", read);
    }
  }
}

void user_input(char *command) {
  kprintf("\n");
  char *input = kalloc(0);
  char *args = kalloc(0);
  u32 i = 0;
  if (strtokn((const char *)command, (byte *)args, ' ', i) == 0) {
    memcopy((byte *)command, (byte *)input, strlen(command) + 1);
  } else {
    i = 0;
    kprintf("\n");
    while (strtokn((const char *)command, (byte *)args, ' ', i) != 0) {
      kprintf("[%d] %s\n", i, args);
      if (i == 0) {
        memcopy((byte *)args, (byte *)input, strlen(args) + 1);
        break;
      }
      ++i;
    }
  }

  if (strcmp(input, "help") == 0)
    printHelp();
  else if (!strcmp(input, "clear")) {
    clearScreen();
  } else if (!strcmp(input, "exit")) {
    clearScreen();
    sys_exit(0);
  } else if (!strcmp(input, "files")) {
    // kprintf("Files start: 0x%x, Files end: 0x%x\n", &files_start,
    // &files_end);
    Elf32_Ehdr *b = (Elf32_Ehdr *)&files_start;
    // print_elf(b);
    Thread *run_me = load_elf(b);
    wake_up_thread(run_me);
    // sys_wait4(run_me->pid);
    //_switch_to_thread(run_me);
  } else if (!strcmp(input, "flag")) {
    asm volatile("cli");
    itaFlag();
    asm volatile("sti");
  } else if (!strcmp(input, "free")) {
    printFree();
  } else if (!strcmp(input, "pci")) {
    checkAllBuses();
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
    // sys_wait4all();
  } else if (!strcmp(input, "cup")) {
    Thread *p;
    for (int i = 0; i < ALLOC_NUM; ++i) {
      MemDesc *thread_mem = kalloc(0);
      LIST_INIT(&thread_mem->vm_areas);
      thread_mem->page_directory = (u32)kalloc(0);
      init_user_paging((u32 *)thread_mem->page_directory);

      p = create_user_thread(&u_simple_proc, thread_mem, NULL, NULL, "u-extra");
      p->nice = 10;
      VMArea *stack =
          create_vmregion(USER_STACK_TOP - PAGE_SIZE, USER_STACK_TOP,
                          PA((u32)p->tcb.user_stack_bot), 0, VMA_STACK);
      list_add_tail(&thread_mem->vm_areas, &stack->head);
    }
    wake_up_thread(p);

    // sys_wait4all();
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
