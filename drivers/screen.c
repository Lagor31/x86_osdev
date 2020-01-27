#include "../cpu/types.h"

#include "../cpu/ports.h"
#include "../kernel/mem.h"
#include "../libc/strings.h"

#include "screen.h"

uint8_t textColor = DEFAULT_ATTR;

void setTextColor(uint8_t fgColor) {
  textColor = (textColor >> 4) << 4 | (0x0f & fgColor);
}
void setBackgroundColor(uint8_t bgColor) { textColor |= 0xf0 & (bgColor << 4); }

void resetScreenColors() { textColor = DEFAULT_ATTR; }

/*  Convert the integer D to a string and save the string in BUF. If
   BASE is equal to ’d’, interpret that D is decimal, and if BASE is
   equal to ’x’, interpret that D is hexadecimal. */
static void itoa(char *buf, int base, int d) {
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
void kprintfColor(uint8_t color, const char *format, ...) {
  setTextColor(color);
  char **arg = (char **)&format;
  int c;
  char buf[20];

  arg++;

  while ((c = *format++) != 0) {
    if (c != '%') {
      printChar(c, textColor);
    } else {
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
          for (; p2 < p + pad; p2++) {
            printChar(pad0 ? '0' : ' ', textColor);
          }
          while (*p) {
            printChar(*p++, textColor);
          }
          break;

        default:
          printChar(*((int *)arg++), textColor);
          break;
      }
    }
  }

  resetScreenColors();
}
/*  Format a string and print it on the screen, just like the libc
   function printf. */
void kprintf(const char *format, ...) {
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

int deleteLastChar() {
  int curOff = getCursorOffset();
  curOff -= 2;
  int row = getOffsetRow(curOff);
  int col = getOffsetCol(curOff);
  printCharAt(row, col, ' ', DEFAULT_ATTR);
  setCursorPos(row, col);
  return getOffset(row, col);
}

/*
Booting with GRUB and timeout=0 disables cursor showing, this enables is
*/
void activateCursor() {
  outb(0x3D4, 0x09);  // set maximum scan line register to 15
  outb(0x3D5, 0x0F);

  outb(0x3D4, 0x0B);  // set the cursor end line to 15
  outb(0x3D5, 0x0F);

  outb(0x3D4,
       0x0A);  // set the cursor start line to 14 and enable cursor visibility
  outb(0x3D5, 0x0C);
}
void setColor(uint8_t c) { textColor = c; }

int getCursorOffset() {
  /* Use the VGA ports to get the current cursor position
   * 1. Ask for high byte of the cursor offset (data 14)
   * 2. Ask for low byte (data 15)
   */
  outb(VGA_CTRL_PORT, 14);
  int offset = inb(VGA_DATA_PORT) << 8; /* High byte: << 8 */
  outb(VGA_CTRL_PORT, 15);
  offset += inb(VGA_DATA_PORT);
  return offset * 2; /* Position * size of character cell */
}

void setCursorPos(int row, int col) { setCursorOffset(getOffset(row, col)); }

void setCursorOffset(int offset) {
  /* Similar to get_cursor_offset, but instead of reading we write data */
  offset /= 2;
  outb(VGA_CTRL_PORT, 14);
  outb(VGA_DATA_PORT, (unsigned char)(offset >> 8));
  outb(VGA_CTRL_PORT, 15);
  outb(VGA_DATA_PORT, (unsigned char)(offset & 0xff));
}

void clearScreen() {
  textColor = DEFAULT_ATTR;
  for (int r = 0; r < VGA_ROWS; r++) {
    clearRow(r);
  }
  setCursorPos(0, 0);
}

void clearRow(int row) {
  for (int c = 0; c < VGA_COLUMNS; ++c) {
    printCharAt(row, c, ' ', textColor);
  }
}

void clearCharAt(int row, int col) {
  char *video_memory = (char *)VGA_ADDRESS;
  int pos = (row * 80 + col) * 2;
  video_memory[pos] = ' ';
  video_memory[pos + 1] = textColor;
}

void kPrintOKMessage(const char *message) {
  kprintf("[");
  kprintfColor(LIGHTGREEN, "OK");
  kprintf("] %s\n", message);
}

void kPrintKOMessage(const char *message) {
  kprintf("[");
  kprintfColor(LIGHTRED, "KO");
  kprintf("] %s\n", message);
}
// Prints char at cursor position
int printChar(char c, char attr) { return printCharAt(-1, -1, c, attr); }

int printCharAt(int row, int col, char c, char attr) {
  unsigned char *vidmem = (unsigned char *)VGA_ADDRESS;
  if (!attr) attr = DEFAULT_ATTR;

  /* Error control: print a red 'E' if the coords aren't right */
  if (col >= VGA_COLUMNS || row >= VGA_ROWS) {
    vidmem[2 * (VGA_COLUMNS) * (VGA_ROWS)-2] = 'E';
    vidmem[2 * (VGA_COLUMNS) * (VGA_ROWS)-1] = DEFAULT_ATTR;
    return getOffset(col, row);
  }

  int offset;
  if (col >= 0 && row >= 0)
    offset = getOffset(row, col);
  else
    offset = getCursorOffset();

  if (c == '\n') {
    row = getOffsetRow(offset);
    offset = getOffset(row + 1, 0);
  } else {
    vidmem[offset] = c;
    vidmem[offset + 1] = textColor;
    offset += 2;
  }

  if (offset >= VGA_ROWS * VGA_COLUMNS * 2) {
    memcopy((uint8_t *)VGA_ADDRESS + getOffset(1, 0), (uint8_t *)VGA_ADDRESS,
            getOffset(VGA_ROWS - 1, VGA_COLUMNS));
    // clearRow(VGA_ROWS - 1);
    offset = getOffset(VGA_ROWS - 1, 0);
  }

  setCursorOffset(offset);
  return offset;
}

int getOffset(int row, int col) { return 2 * (row * VGA_COLUMNS + col); }

int getOffsetRow(int offset) { return offset / (2 * VGA_COLUMNS); }

int getOffsetCol(int offset) {
  return (offset - (getOffsetRow(offset) * 2 * VGA_COLUMNS)) / 2;
}