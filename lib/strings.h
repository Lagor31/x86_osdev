#ifndef STRINGS_H
#define STRINGS_H

#include "../cpu/types.h"

u32 strlen(char *);
void intToAscii(int, char *);
void reverse(char s[]);
void backspace(char s[]);
void append(char s[], char n);
u32 strcmp(char s1[], char s2[]);
void hex_to_ascii(int n, char str[]);

#endif