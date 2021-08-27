#include "libc.h"

char* par = "Hello from parent!\n";
char* child = "Hello from child!\n";
char* childbye = "Bye from child!\n";
char* parbye = "Bye from parent!\n";

char* mess = "Child cloned!\n";
char* m = "Print!\n";

void _start() {
  int i = 0;
  unsigned r = random(2000);
  sleepms(r);
  write(1, par, 8);
  unsigned pid = clone(CLONE_VM);

  // Child
  if (pid == 0) {
    write(1, child, 8);
    sleepms(1000);
    write(1, childbye, 8);
    exit(0);
  } else {
    // Father
    sleepms(3000);
    wait4(pid);
    write(1, parbye, 8);
    exit(0);
  }

  exit(0);
}