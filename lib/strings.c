#include "../cpu/types.h"

#include "strings.h"
#include "../mem/mem.h"

u32 strlen(char s[]) {
  int i = 0;
  while (s[i] != '\0') ++i;
  return i;
}

void intToAscii(int n, char str[]) {
  int i, sign;
  if ((sign = n) < 0) n = -n;
  i = 0;
  do {
    str[i++] = n % 10 + '0';
  } while ((n /= 10) > 0);

  if (sign < 0) str[i++] = '-';
  str[i] = '\0';
  reverse(str);
}

void hex_to_ascii(int n, char str[]) {
  append(str, '0');
  append(str, 'x');
  char zeros = 0;

  int32_t tmp;
  int i;
  for (i = 28; i > 0; i -= 4) {
    tmp = (n >> i) & 0xF;
    if (tmp == 0 && zeros == 0) continue;
    zeros = 1;
    if (tmp > 0xA)
      append(str, tmp - 0xA + 'a');
    else
      append(str, tmp + '0');
  }

  tmp = n & 0xF;
  if (tmp >= 0xA)
    append(str, tmp - 0xA + 'a');
  else
    append(str, tmp + '0');
}

/* K&R */
void reverse(char s[]) {
  int c, i, j;
  for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

void append(char s[], char n) {
  int len = strlen(s);
  s[len] = n;
  s[len + 1] = '\0';
}

void backspace(char s[]) {
  int len = strlen(s);
  if (len > 0) s[len - 1] = '\0';
}

/* K&R
 * Returns <0 if s1<s2, 0 if s1==s2, >0 if s1>s2 */
u32 strcmp(char s1[], char s2[]) {
  int i;
  for (i = 0; s1[i] == s2[i]; i++) {
    if (s1[i] == '\0') return 0;
  }
  return s1[i] - s2[i];
}

u32 strtokn(const char *input, char *out_token, char delim, u32 tok_num) {
  u32 count = 0;
  u32 i = 0;
  u32 last_tkn_start = 0;
  bool found = FALSE;
  if (input == NULL) {
    out_token[0] = '\0';
    return 0;
  }

  while (input[i] != '\0') {
    if (input[i] == delim) {
      found = TRUE;
      if (tok_num == count) {
        // Found token in the requested position
        memcopy((byte *)(input + last_tkn_start), out_token,
                i - last_tkn_start);
        out_token[i - last_tkn_start] = '\0';
        return i - last_tkn_start;
      }
      count++;
      while (input[i] == delim) ++i;
      last_tkn_start = i;
    }
    ++i;
  }

  if (found && tok_num == count) {
    // Found token in the requested position
    memcopy((byte *)(input + last_tkn_start), out_token, i - last_tkn_start);
    out_token[i - last_tkn_start] = '\0';
    return i - last_tkn_start;
  } else {
    out_token[0] = '\0';
    return 0;
  }
}

char *strtok(register char *s, register const char *delim, char *out_tok) {
  register char *spanp;
  register int c, sc;
  char *tok;
  // static char *last;

  if (s == NULL) return (NULL);

  /*
   * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
   */
cont:
  c = *s++;
  for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
    if (c == sc) goto cont;
  }

  if (c == 0) { /* no non-delimiter characters */
    // last = NULL;
    return (NULL);
  }
  tok = s - 1;

  /*
   * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
   * Note that delim must have one NUL; we stop if we see that, too.
   */
  for (;;) {
    c = *s++;
    spanp = (char *)delim;
    do {
      if ((sc = *spanp++) == c) {
        if (c == 0)
          s = NULL;
        else
          s[-1] = 0;

        memcopy(tok, out_tok, c);
        // last = s;
        return (s);
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
}
