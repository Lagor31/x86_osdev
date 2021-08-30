#include "libc.h"

char* par = "Hello from parent! %d";
char* child = "Hello from child! %d";
char* childbye = "Bye from child!";
char* parbye = "Bye from parent!";

char* mess = "Child cloned!";
char* m = "Print!";

void _start() {
  unsigned r = random(2000);
  sleepms(r);
  // write(1, par, 8);
  printf(par, getpid());
  unsigned pid = clone(CLONE_VM);

  // Child
  if (pid == 0) {
    int i = 0;
    while (i++ < 10) {
      printf(child, getpid());
      sleepms(1000);
    }
    printf(childbye);
    exit(0);
  } else {
    // Father
    sleepms(3000);
    wait4(pid);
    // write(1, parbye, 8);
    printf(parbye);
    exit(0);
  }

  exit(0);
}