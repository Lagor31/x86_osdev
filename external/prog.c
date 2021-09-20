#include "libc.h"

char* par = "Hello from parent!\n";
char* child = "Hello from child!\n";
char* childbye = "Bye from child!\n";
char* parbye = "Bye from parent!\n";

char* mess = "Child cloned!";
char* m = "Print!";

void _start() {
  unsigned r = random(2000);
  sleepms(r);
  // write(1, par, 8);
  printf(par);
  unsigned pid = clone(CLONE_VM);

  // Child
  if (pid == 0) {
    int i = 0;
    while (i++ < 10) {
      printf(child);
      sleepms(1000);
    }
    printf(childbye);
    exit(0);
  } else {
    // Father
    sleepms(3000);
    //kill(pid, SIGKILL);
    wait4(pid);
    // write(1, parbye, 8);
    printf(parbye);
    exit(0);
  }

  exit(0);
}