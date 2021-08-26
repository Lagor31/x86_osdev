#ifndef LIBC_H
#define LIBC_H

#define SYS_EXIT 1
#define SYS_PRINTF 2
#define SYS_SLEEPMS 3
#define SYS_WAIT4ALL 4
#define SYS_WAIT4 5
#define SYS_RANDOM 6
#define SYS_WRITE 7
#define SYS_GETPID 8


extern unsigned int _system_call0(unsigned int num);

extern unsigned int _system_call1(unsigned int num, unsigned int p1);

extern unsigned int _system_call2(unsigned int num, unsigned int p1,
                                  unsigned int p2);

extern unsigned int _system_call3(unsigned int num, unsigned int p1,
                                  unsigned int p2, unsigned int p3);

void exit(int exit_code);
void sleepms(unsigned ms);
void printf();
unsigned int random(unsigned int max);
unsigned write(unsigned fd, char* buf, unsigned len);
unsigned getpid();
#endif