#include "binaries.h"

void idle() {
  while (TRUE) {
    hlt();
    reschedule();
  }
}
