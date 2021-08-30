#include "binaries.h"
#include "../kernel/files.h"
void login() {
  while (TRUE) {
    clearScreen();
    resetScreenColors();
    kprintf("\nlogin root: ");
    char prev_read = '\0';
    char *my_buf = kalloc(0);
    memset((byte *)my_buf, '\0', PAGE_SIZE);
    while (TRUE) {
      int child_num = 0;
      List *l = NULL;
      FD *child_fd = NULL;
      char read = read_stdin();

      list_for_each(l, &current_thread->children) {
        ++child_num;
        Thread *c = list_entry(l, Thread, siblings);
        child_fd = c->std_files[0];
        if (read == CTRL) {
          prev_read = read;
          write_byte_stream(child_fd, read);
          break;
        } else {
          write_byte_stream(child_fd, read);
        }
      }

      if (read == 'n' && prev_read == CTRL) {
        kprintf("Special code received\n");
      }

      if (child_num > 0) {
        prev_read = read;
        continue;
      }

      if (read == '\n') {
        if (!strcmp(my_buf, "root")) {
          kprintf("\n\nWelcome root!\n");
          Thread *p;
          p = create_kernel_thread(&shell, NULL, "shell");
          p->nice = 0;
          p->std_files[0] = create_char_device("shell_in", 5);
          wake_up_thread(p);
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
      } else if (read == CTRL) {
        continue;
      } else {
        append(my_buf, read);
        kprintf("*");
      }
      prev_read = read;
    }
  }
}
