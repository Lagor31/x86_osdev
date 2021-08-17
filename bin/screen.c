
#include "binaries.h"
#include "../kernel/files.h"
void screen_refresh() {
  kernel_init_ok = TRUE;
  // setCursorOffset(0);

  unsigned char *vidmem = (unsigned char *)VA(VGA_ADDRESS);

  while (TRUE) {
    // kprintf("1) PID %d\n", current_proc->pid);
    // printProc(current_proc);
    // clearScreen();

    int row = 0;
    int col = 0;
    disable_int();
    for (row = 0; row < VGA_ROWS; row++) {
      for (col = 0; col < VGA_COLUMNS; col++) {
        // set_pos_block_nb(stdout, getOffset(row, col));
        u32 off = getOffset(row, col);
        // stdout[off] =  c;
        u8 c = stdout->buffer[off];
        // set_pos_block_nb(stdout, getOffset(row, col) + 1);
        u8 att = stdout->buffer[off + 1];
        /* if (c != '\0') {
          // setTextColor(att);
          // setCursorOffset(i);
          // printCharAt(row, col, c, att);
          vidmem[off] = c;
          vidmem[off + 1] = att;
        } else {
          vidmem[off] = ' ';
          vidmem[off + 1] = att;
        } */
        if (vidmem[off] != c) vidmem[off] = c;
        if (vidmem[off + 1] != att) vidmem[off + 1] = att;
      }
    }
    enable_int();
    sleep_ms(16);
  }
}
