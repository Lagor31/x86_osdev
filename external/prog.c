#include "libc.h"

char* s = "Hello from usermode!\n";
void _start() {
  int i = 0;
  while (i++ < 200) {
    unsigned r = random(6000);
    sleepms(r);
    // printf();
    // char t = s[0];

    //write(1, s, 8);
    unsigned pid = getpid();
  }
  exit(0);
}