#include "libc.h"

char* s = "Hello from usermode!\n";
void _start() {
  int i = 0;
  while (i++ < 200) {
    unsigned r = random(6000);
    sleepms(r);
    write(1, s, 8);
  }
  exit(0);
}