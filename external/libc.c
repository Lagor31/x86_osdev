#include "libc.h"

void exit(int exit_code) { _system_call1(SYS_EXIT, exit_code); }

void sleepms(unsigned ms) { _system_call1(SYS_SLEEPMS, ms); }

unsigned int random(unsigned int max) { return _system_call1(SYS_RANDOM, max); }

unsigned write(unsigned fd, char* buf, unsigned len) {
  return _system_call3(SYS_WRITE, fd, (unsigned)buf, len);
}

unsigned getpid() { return _system_call0(SYS_GETPID); }

unsigned clone(unsigned flags) { return _system_call1(SYS_CLONE, flags); }

void wait4(unsigned pid) { _system_call1(SYS_WAIT4, pid); }

unsigned strlen(char s[]) {
  int i = 0;
  while (s[i] != '\0') ++i;
  return i;
}

void printf(char* s) {
  int i = 0;
  int l = strlen(s);
  char att = 0xf5;;
  while (i < l) {
    write(1, &s[i], 1);
    write(1, &att, 1);
    ++i;
  }
}