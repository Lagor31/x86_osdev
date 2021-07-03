#include "../cpu/types.h"

#include "../utils/list.h"

#include "../cpu/gdt.h"
#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../kernel/kernel.h"
#include "../libc/constants.h"
#include "../libc/functions.h"
#include "../libc/strings.h"
#include "../mem/paging.h"
#include "../proc/proc.h"
#include "../utils/utils.h"

#include "screen.h"

#include "keyboard.h"

Stdin stdin;

void init_stdin() {
  // 16 Kb
  stdin.buffer = (char *)normal_page_alloc(2);
  memset((byte *)stdin.buffer, 0, PAGE_SIZE * 2);
  stdin.read_lock = make_lock();
  stdin.read_lock->state = LOCK_LOCKED;
  stdin.available = 0;
  stdin.last = 0;
}

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
  get_lock(stdin.read_lock);

  char out = stdin.buffer[stdin.last++];
  stdin.available--;
  if (stdin.available <= 0) {
    memset((byte *)stdin.buffer, '\0', PAGE_SIZE * 2);
    stdin.last = 0;
  } else
    unlock(stdin.read_lock);

  return out;

  /*   char *out_string = normal_page_alloc(2);
    memcopy((byte *)stdin.buffer, (byte *)out_string, stdin.available + 1);
    memset((byte *)stdin.buffer, 0, PAGE_SIZE * 2);
    out_string[stdin.available + 1] = '\0';
    stdin.available = 0;
    return out_string; */
}

static void keyboard_callback(registers_t *regs) {
  /*   u8 userMode = FALSE;
    if ((regs->cs & 0b11) == 3) userMode = TRUE; */

  outb(PIC_CMD_RESET, PORT_PIC_MASTER_CMD); // select register C

  /* The PIC leaves us the scancode in port 0x60 */
  u8 scancode = inb(0x60);
  if (scancode > SC_MAX)
    return;
  char c = '\0';

  if (scancode == BACKSPACE)
    c = (char)BACKSPACE;
  else if (scancode == ENTER)
    c = '\n';
  else
    c = sc_ascii[(int)scancode];

  Work *w = normal_page_alloc(0);
  w->c = c;
  list_add(&kwork_queue, &w->work_queue);

  regs->eflags |= 0x200;
  tss.cs = 0x10;
  tss.esp0 = getRegisterValue(ESP);
}

void init_keyboard() { register_interrupt_handler(IRQ1, keyboard_callback); }
