
#include "binaries.h"
#include "../kernel/files.h"
void screen_refresh() {
  kernel_init_ok = TRUE;

  unsigned char *vidmem = (unsigned char *)VA(VGA_ADDRESS);

  while (TRUE) {
    int row = 0;
    int col = 0;
    bool p = disable_int();
    for (row = 0; row < VGA_ROWS; row++) {
      for (col = 0; col < VGA_COLUMNS; col++) {
        u32 off = getOffset(row, col);
        u8 c = stdout->buffer[off];
        u8 att = stdout->buffer[off + 1];
        if (vidmem[off] != c) vidmem[off] = c;
        if (vidmem[off + 1] != att) vidmem[off + 1] = att;
      }
    }
    enable_int(p);
    sleep_ms(16);
  }
}
