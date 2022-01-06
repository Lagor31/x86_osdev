#include "../cpu/types.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../lib/strings.h"
#include "../lock/lock.h"
#include "../kernel/files.h"
#include "../kernel/scheduler.h"
#include "../net/net.h"

#include "thread.h"

List kwork_queue;
Thread *kwork_thread;

void init_work_queue() { LIST_INIT(&kwork_queue); }

void work_queue_thread() {
  while (TRUE) {
    List *l;
  net_work:
    list_for_each(l, &kwork_queue) {
      Work *p1 = (Work *)list_entry(l, Work, work_queue);
      // if (p1->type == 0) kprintf("Received network packet\n");
      list_remove(&p1->work_queue);
      print_ethernet_packet(p1->data);
      ffree(p1->data);
      ffree(p1);
      goto net_work;
    }

    while (key_buf_avail > 0) {
      char scancode = keyboard_buffer[key_buf_last++ % 1024];
      char c = '\0';
      if (scancode == ENTER)
        c = '\n';
      else if (scancode == BACKSPACE)
        c = (char)BACKSPACE;
      else if (scancode == CTRL)
        c = (char)CTRL;
      else
        c = sc_ascii[(int)scancode];
      write_byte_stream(stdin, c);
      key_buf_avail--;
      if (key_buf_avail <= 0) {
        memset((byte *)keyboard_buffer, '\0', 1024);
        key_buf_last = 0;
      }
    }

    sleep_thread(current_thread);
    reschedule();
  }
}