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
  char c = '\0';
  // kprintf("scancode: %d", scancode);
  if (scancode == BACKSPACE)
    c = (char)BACKSPACE;
  else if (scancode == CTRL)
    c = (char)CTRL;
  else if (scancode == ENTER)
    c = '\n';
  else
    c = sc_ascii[(int)scancode];

  Work *w = kernel_page_alloc(0);
  w->c = c;
  list_add_head(&kwork_queue, &w->work_queue);
  // if (work_queue_lock->state == LOCK_LOCKED) unlock(work_queue_lock);
  wake_up_thread(kwork_thread);
  UNUSED(regs);
}

void init_keyboard() { register_interrupt_handler(IRQ1, keyboard_callback); }
