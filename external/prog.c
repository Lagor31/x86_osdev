#include "libc.h"

int x;
void _start() {
  int i = 0;
  while (i++ < 200) {
    unsigned r = random(6000);
    x++;
    sleepms(r);
    printf();
  }
  exit(0);
}