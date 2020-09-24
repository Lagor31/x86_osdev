#include "../cpu/types.h"

#include "../utils/list.h"

#include "../cpu/isr.h"
#include "../cpu/ports.h"
#include "../kernel/kernel.h"
#include "../mem/paging.h"
#include "../libc/constants.h"
#include "../libc/functions.h"
#include "../libc/strings.h"
#include "../utils/utils.h"

#include "../cpu/gdt.h"

#include "screen.h"

#include "keyboard.h"

static char key_buffer[256];

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

static void keyboard_callback(registers_t *regs) {
  uint8_t userMode = FALSE;
  if ((regs->cs & 0b11) == 3) userMode = TRUE;

  if (userMode)
    _loadPageDirectory((uint32_t *)PA((uint32_t)kernel_page_directory));

  outb(PIC_CMD_RESET, PORT_PIC_MASTER_CMD);  // select register C

  /* The PIC leaves us the scancode in port 0x60 */
  uint8_t scancode = inb(0x60);
  if (scancode > SC_MAX) return;
  if (scancode == BACKSPACE) {
    if (strlen(key_buffer) > 0) deleteLastChar();
    backspace(key_buffer);
  } else if (scancode == ENTER) {
    kprintf("\n");
    user_input(key_buffer); /* kernel-controlled function */
    key_buffer[0] = '\0';
  } else {
    char letter = sc_ascii[(int)scancode];
    /* Remember that kprint only accepts char[] */
    char str[2] = {letter, '\0'};
    append(key_buffer, letter);
    kprintf(str);
  }

  regs->eflags |= 0x200;
  tss.cs = 0x10;
  tss.esp0 = getRegisterValue(ESP);
  if (userMode) {
    user_page_directory[0] &= 0xFFFFFFDF;
    user_page_directory[1] &= 0xFFFFFFDF;
    _loadPageDirectory((uint32_t *)PA(
        (uint32_t)&user_page_directory));  // REmove A(ccessed) bit
  }
}

void init_keyboard() { register_interrupt_handler(IRQ1, keyboard_callback); }
