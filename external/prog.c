#include "libc.h"

char* par = "Hello from parent!";
char* child = "Hello from child!";
char* childbye = "Bye from child!";
char* parbye = "Bye from parent!";

char* mess = "Child cloned!\n";
char* m = "Print!\n";

void _start() {
  unsigned r = random(2000);
  sleepms(r);
  // write(1, par, 8);
  printf(par);
  unsigned pid = clone(CLONE_VM);

  // Child
  if (pid == 0) {
    printf(child);
    sleepms(1000);
    printf(childbye);
    exit(0);
  } else {
    // Father
    sleepms(3000);
    wait4(pid);
    // write(1, parbye, 8);
    exit(0);
  }

  exit(0);
}