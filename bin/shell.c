#include "binaries.h"
#include "../lib/list.h"
#include "../kernel/files.h"
#include "../drivers/cmos.h"
#include "../drivers/pci.h"
#include "../kernel/kernel.h"
#include "../kernel/elf.h"
#include "../kernel/timer.h"
#include "../mem/mem.h"
#include "../drivers/rtl8139.h"
#include "../mem/slab.h"
#include "../net/arp.h"
#include "../mem/buddy_new.h"

void *fm;

void *nor;
void print_prompt() {
  setTextColor(LIGHTGREEN);
  setBackgroundColor(BLACK);
  kprintf("\n%s@%s # ", current_thread->owner->username, HOSTNAME);
  resetScreenColors();
}

BuddyBlockNew *alloc_b[ALLOC_NUM];

void shell() {
  char prev_read = '\0';

  resetScreenColors();

  print_prompt();
  char *my_buf = kalloc_page(0);
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
  char *input = kalloc_page(0);
  char *args = kalloc_page(0);
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
  } else if (!strcmp(input, "timers")) {
    List *t;
    bool pi = disable_int();
    list_for_each(t, &kernel_timers) {
      Timer *activeT = list_entry(t, Timer, q);
      kprintf("Timer of %s\n", activeT->thread->command);
    }
    enable_int(pi);
  } else if (!strcmp(input, "slab")) {
    /* for (size_t i = 0; i < 10; i++) {
      u32 s = rand() % 20 + 1;
      void *p = kmalloc(s);
      kprintf("Pointer: 0x%x S: %d\n", p, s);
    } */
    // kprintf("Buf %d Slab %d\n", sizeof(Buf), sizeof(Slab));
    u32 *num[4];
    for (size_t i = 0; i < 4; i++) {
      num[i] = (u32 *)salloc(sizeof(u32));
      *num[i] = 31 + i;
    }

    for (size_t i = 0; i < 4; i++) {
      kprintf("0x%x = %d \n", num[i], *num[i]);
    }
    kprintf("\n");
    for (size_t i = 0; i < 4; i++) {
      sfree(num[i]);
    }

    void *b = salloc(256);
    kprintf("256 - 0x%x\n", b);
    sfree(b);
    salloc(256);
    kprintf("256 - 0x%x\n", b);
    sfree(b);
    salloc(256);
    kprintf("256 - 0x%x\n", b);
    sfree(b);
    salloc(256);
    kprintf("256 - 0x%x\n", b);
    sfree(b);

  } else if (!strcmp(input, "si")) {
    List *p;
    kprintf("Free:\n");
    // get_lock(slab_lock);
    List *tem;
    list_for_each_safe(p, tem, &kMemCache.free) {
      Slab *s = list_entry(p, Slab, head);
      if (s->pinned) setTextColor(RED);
      kprintf("- Cache: %d %d/%d\n", s->size, s->alloc, s->tot);
      resetScreenColors();
    }
    kprintf("Used:\n");
    list_for_each_safe(p, tem, &kMemCache.used) {
      Slab *s = list_entry(p, Slab, head);
      if (s->pinned) setTextColor(RED);
      kprintf("- Cache: %d %d/%d\n", s->size, s->alloc, s->tot);
      resetScreenColors();
    }

    kprintf("Empty:\n");
    list_for_each_safe(p, tem, &kMemCache.empty) {
      Slab *s = list_entry(p, Slab, head);
      if (s->pinned) setTextColor(RED);
      kprintf("- Cache: %d %d/%d\n", s->size, s->alloc, s->tot);
      resetScreenColors();
    }
    // unlock(slab_lock);
  } else if (!strcmp(input, "exit")) {
    // clearScreen();
    sys_exit(0);
  } else if (!strcmp(input, "f")) {
    // kprintf("Files start: 0x%x, Files end: 0x%x\n", &files_start,
    // &files_end);
    Elf32_Ehdr *b = (Elf32_Ehdr *)&files_start;
    // print_elf(b);

    for (size_t i = 0; i < ALLOC_NUM; i++) {
      Thread *run_me = load_elf(b);
      wake_up_thread(run_me);
    }

    // sys_wait4(run_me->pid);
    //_switch_to_thread(run_me);
  } else if (!strcmp(input, "flag")) {
    asm volatile("cli");
    itaFlag();
    asm volatile("sti");
  } else if (!strcmp(input, "free")) {
    printFree();
  } else if (!strcmp(input, "tb")) {
    bool pi = disable_int();

    void *alloc_ptr[ALLOC_NUM];
    for (u32 i = 0; i < ALLOC_NUM; ++i) {
      u32 r = rand() % (8192);
      // kprintf("%d) Size: %d -> ", i, r);
      alloc_ptr[i] = fmalloc_new(r);
      // ffree_new(alloc_ptr[i]);

      // free_buddy_new(alloc_b[i], &fast_buddy_new);
      // print_buddy_new(alloc_b[i]);
    }
    // kprintfColor(GREEN, "Alloc done!\n");

    for (u32 o = 0; o <= MAX_ORDER; ++o) print_buddy_status(o, &fast_buddy_new);
    for (u32 k = 0; k < ALLOC_NUM; ++k) {
      //    kprintf("%d) Freeing\n", k);
      // if (k % 3 == 0) continue;
      // print_buddy_new(alloc_b[k]);
      ffree_new(alloc_ptr[k]);
    }
    kprintfColor(LIGHTBLUE, "Freeing done!\n");
    enable_int(pi);

  } else if (!strcmp(input, "buddy")) {
    bool pi = disable_int();
    for (u32 i = 0; i < ALLOC_NUM; ++i) {
      u32 r = rand() % (MAX_ORDER - 7);
      // kprintf("%d) Size: %d -> ", i, r);
      alloc_b[i] = get_buddy_new(r, &fast_buddy_new);
      // free_buddy_new(alloc_b[i], &fast_buddy_new);
      // print_buddy_new(alloc_b[i]);
    }
    kprintfColor(GREEN, "Alloc done!\n");

    for (u32 o = 0; o <= MAX_ORDER; ++o) print_buddy_status(o, &fast_buddy_new);
    for (u32 k = 0; k < ALLOC_NUM; ++k) {
      //    kprintf("%d) Freeing\n", k);
      // if (k % 3 == 0) continue;
      // print_buddy_new(alloc_b[k]);
      free_buddy_new(alloc_b[k], &fast_buddy_new);
    }
    kprintfColor(BLUE, "Freeing done!\n");
    enable_int(pi);
    // free_buddy_new(b);
  } else if (!strcmp(input, "bi")) {
    bool pi = disable_int();
    for (u32 o = 0; o <= MAX_ORDER; ++o) {
      print_buddy_status(o, &fast_buddy_new);
    }
    enable_int(pi);
  } else if (!strcmp(input, "mac")) {
    print_mac_address();
  } else if (!strcmp(input, "tx")) {
   /*  byte dest[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    byte src[] = {0xfe, 0xfe, 0xde, 0x32, 0x12, 0x34};
    u16 type = 0x0608;
    byte *data = kmalloc(30);
    memset(data, 31, 30) */;
    // send_ethernet_packet(dest, type, data, 10);
    byte ip[4] = {192, 168, 1, 254};
    send_arp_request(ip);
    // rtl8139_send_packet(arp, 60);
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
    // clearScreen();
  } else if (!strcmp(input, "ps")) {
    Thread *p = NULL;
    p = create_kernel_thread(&ps, NULL, "ps");
    p->nice = 0;
    wake_up_thread(p);
    sys_wait4(p->pid);
  } else if (!strcmp(input, "k")) {
    Thread *p;
    // for (int i = 0; i < ALLOC_NUM; ++i) {
    p = create_kernel_thread(&k_simple_proc, NULL, "k-extra");
    p->nice = 10;
    wake_up_thread(p);
    //}
    // sys_wait4all();
  } else if (!strcmp(input, "u")) {
    Thread *p;
    for (int i = 0; i < ALLOC_NUM; ++i) {
      MemDesc *thread_mem = kmalloc(sizeof(MemDesc));
      LIST_INIT(&thread_mem->vm_areas);
      thread_mem->page_directory = (u32)kalloc_page(0);
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

  kfree_page(input);
  kfree_page(args);
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
