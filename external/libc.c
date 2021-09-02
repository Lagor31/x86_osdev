#include "libc.h"

unsigned textColor = 0x0B;

void exit(int exit_code) { _system_call1(SYS_EXIT, exit_code); }

void sleepms(unsigned ms) { _system_call1(SYS_SLEEPMS, ms); }

unsigned int random(unsigned int max) { return _system_call1(SYS_RANDOM, max); }

unsigned write(unsigned fd, char *buf, unsigned len) {
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

void printfLib(const char *format, ...) {
  char **arg = (char **)&format;
  int c;
  char buf[40];

  arg++;

  while ((c = *format++) != 0) {
    if (c != '%')
      printChar(c, textColor);
    else {
      char *p, *p2;
      int pad0 = 0, pad = 0;

      c = *format++;
      if (c == '0') {
        pad0 = 1;
        c = *format++;
      }

      if (c >= '0' && c <= '9') {
        pad = c - '0';
        c = *format++;
      }

      switch (c) {
        case 'd':
        case 'u':
        case 'x':
          itoa(buf, c, *((int *)arg++));
          p = buf;
          goto string;
          break;

        case 's':
          p = *arg++;
          if (!p) p = "(null)";

        string:
          for (p2 = p; *p2; p2++)
            ;
          for (; p2 < p + pad; p2++) printChar(pad0 ? '0' : ' ', textColor);
          while (*p) printChar(*p++, textColor);
          break;

        default:
          printChar(*((int *)arg++), textColor);
          break;
      }
    }
  }
}

void itoa(char *buf, int base, int d) {
  char *p = buf;
  char *p1, *p2;
  unsigned long ud = d;
  int divisor = 10;

  /*  If %d is specified and D is minus, put ‘-’ in the head. */
  if (base == 'd' && d < 0) {
    *p++ = '-';
    buf++;
    ud = -d;
  } else if (base == 'x')
    divisor = 16;

  /*  Divide UD by DIVISOR until UD == 0. */
  do {
    int remainder = ud % divisor;

    *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
  } while (ud /= divisor);

  /*  Terminate BUF. */
  *p = 0;

  /*  Reverse BUF. */
  p1 = buf;
  p2 = p - 1;
  while (p1 < p2) {
    char tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
    p1++;
    p2--;
  }
}

void printChar(char c, char attr) {
  write(1, &c, 1);
  write(1, &attr, 1);
}

void printf1(char *s) {
  int i = 0;
  int l = strlen(s);
  char att = 0xf5;
  while (i < l) {
    write(1, &s[i], 1);
    write(1, &att, 1);
    ++i;
  }
}

void printf(char *s) { _system_call1(SYS_PRINTF, (unsigned)s); }