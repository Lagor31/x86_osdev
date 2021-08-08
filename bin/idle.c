#include "binaries.h"

void idle() {
  while (TRUE) {
    hlt();
    Thread *n = do_schedule();
    wake_up_thread(n);
    _switch_to_thread(n);
  }
}
