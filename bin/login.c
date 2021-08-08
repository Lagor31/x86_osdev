#include "binaries.h"

void login() {
  while (TRUE) {
    kprintf("\nlogin root: ");

    char *my_buf = normal_page_alloc(0);
    memset((byte *)my_buf, '\0', PAGE_SIZE);
    while (TRUE) {
      char read = read_stdin();
      if (read == '\n') {
        if (!strcmp(my_buf, "root")) {
          kprintf("\n\nWelcome root!\n");
          Thread *p;
          p = create_kernel_thread(&shell, NULL, "shell");
          p->nice = 0;
          wake_up_thread(p);
          sys_wait4(p->pid);
          clearScreen();
          // setCursorPos(1, 0);
          kprintf("\nlogin root: ");
        } else {
          kprintf("\nInvalid login!");
          kprintf("\nlogin root: ");
        }

        memset((byte *)my_buf, '\0', PAGE_SIZE);
      } else if (read == BACKSPACE) {
        if (strlen(my_buf) > 0) {
          backspace(my_buf);
          deleteLastChar();
        }
      } else {
        append(my_buf, read);
        kprintf("*");
      }
    }
  }
}