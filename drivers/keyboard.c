#include "../cpu/types.h"

#include "../lib/list.h"

#include "../cpu/gdt.h"
#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../kernel/kernel.h"
#include "../lib/constants.h"
#include "../lib/strings.h"
#include "../mem/paging.h"
#include "../proc/thread.h"
#include "../lib/utils.h"

#include "screen.h"

#include "keyboard.h"
#include "../kernel/files.h"

#define SC_MAX 57

char keyboard_buffer[1024] = {0};
u32 key_buf_avail = 0;
u32 key_buf_last = 0;

const char *sc_name[] = {
    "ERROR",     "Esc",     "1", "2", "3", "4",      "5",
    "6",         "7",       "8", "9", "0", "-",      "=",
    "Backspace", "Tab",     "Q", "W", "E", "R",      "T",
    "Y",         "U",       "I", "O", "P", "[",      "]",
    "Enter",     "Lctrl",   "A", "S", "D", "F",      "G",
    "H",         "J",       "K", "L", ";", "'",      "`",
    "LShift",    "\\",      "Z", "X", "C", "V",      "B",
    "N",         "M",       ",", ".", "/", "RShift", "Keypad *",
    "LAlt",      "Spacebar"};
const char sc_ascii[] = {
    '?', '?', '1', '2', '3', '4', '5', '6', '7', '8', '9',  '0', '-', '=',  '?',
    '?', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',  '[', ']', '?',  '?',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '?', '?',  '?', ' '};

char read_stdin() {
  if (current_thread->std_files[0] != NULL)
    return read_byte_stream(current_thread->std_files[0]);
  else
    return read_byte_stream(stdin);
}

static void keyboard_callback(registers_t *regs) {
  outb(PIC_CMD_RESET, PORT_PIC_MASTER_CMD);  // select register C

  /* The PIC leaves us the scancode in port 0x60 */
  u8 scancode = inb(0x60);
  if (scancode > SC_MAX) return;

  append(keyboard_buffer, scancode);
  key_buf_avail++;
  wake_up_thread(kwork_thread);
  // kprintf("K!\n");
  UNUSED(regs);
}

void init_keyboard() { register_interrupt_handler(IRQ1, keyboard_callback); }
