#include "types.h"
#include "../utils/list.h"

#include "../kernel/kernel.h"

#include "../cpu/gdt.h"
#include "../kernel/kernel.h"

#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../mem/paging.h"
#include "../libc/constants.h"
#include "../libc/strings.h"
#include "../utils/utils.h"

#include "idt.h"
#include "ports.h"
#include "timer.h"

#include "isr.h"

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define PIC_READ_IRR 0x0a /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR 0x0b /* OCW3 irq service next CMD read */

isr_t interrupt_handlers[256];

/* Can't do this with a loop because we need the address
 * of the function names */
void isr_install() {
  set_idt_gate(0, (uint32_t)isr0);
  set_idt_gate(1, (uint32_t)isr1);
  set_idt_gate(2, (uint32_t)isr2);
  set_idt_gate(3, (uint32_t)isr3);
  set_idt_gate(4, (uint32_t)isr4);
  set_idt_gate(5, (uint32_t)isr5);
  set_idt_gate(6, (uint32_t)isr6);
  set_idt_gate(7, (uint32_t)isr7);
  set_idt_gate(8, (uint32_t)isr8);
  set_idt_gate(9, (uint32_t)isr9);
  set_idt_gate(10, (uint32_t)isr10);
  set_idt_gate(11, (uint32_t)isr11);
  set_idt_gate(12, (uint32_t)isr12);
  set_idt_gate(13, (uint32_t)isr13);
  set_idt_gate(14, (uint32_t)isr14);
  set_idt_gate(15, (uint32_t)isr15);
  set_idt_gate(16, (uint32_t)isr16);
  set_idt_gate(17, (uint32_t)isr17);
  set_idt_gate(18, (uint32_t)isr18);
  set_idt_gate(19, (uint32_t)isr19);
  set_idt_gate(20, (uint32_t)isr20);
  set_idt_gate(21, (uint32_t)isr21);
  set_idt_gate(22, (uint32_t)isr22);
  set_idt_gate(23, (uint32_t)isr23);
  set_idt_gate(24, (uint32_t)isr24);
  set_idt_gate(25, (uint32_t)isr25);
  set_idt_gate(26, (uint32_t)isr26);
  set_idt_gate(27, (uint32_t)isr27);
  set_idt_gate(28, (uint32_t)isr28);
  set_idt_gate(29, (uint32_t)isr29);
  set_idt_gate(30, (uint32_t)isr30);
  set_idt_gate(31, (uint32_t)isr31);

  // Remap the PIC
  outb(0x20, 0x11);
  outb(0xA0, 0x11);
  outb(0x21, 0x20);
  outb(0xA1, 0x28);
  outb(0x21, 0x04);
  outb(0xA1, 0x02);
  outb(0x21, 0x01);
  outb(0xA1, 0x01);
  outb(0x21, 0x0);
  outb(0xA1, 0x0);

  // Install the IRQs
  set_idt_gate(32, (uint32_t)irq0);
  set_idt_gate(33, (uint32_t)irq1);
  set_idt_gate(34, (uint32_t)irq2);
  set_idt_gate(35, (uint32_t)irq3);
  set_idt_gate(36, (uint32_t)irq4);
  set_idt_gate(37, (uint32_t)irq5);
  set_idt_gate(38, (uint32_t)irq6);
  set_idt_gate(39, (uint32_t)irq7);
  set_idt_gate(40, (uint32_t)irq8);
  set_idt_gate(41, (uint32_t)irq9);
  set_idt_gate(42, (uint32_t)irq10);
  set_idt_gate(43, (uint32_t)irq11);
  set_idt_gate(44, (uint32_t)irq12);
  set_idt_gate(45, (uint32_t)irq13);
  set_idt_gate(46, (uint32_t)irq14);
  set_idt_gate(47, (uint32_t)irq15);

  set_idt();  // Load with ASM
}

/* To print the message which defines every exception */
char *exception_messages[] = {"Division By Zero",
                              "Debug",
                              "Non Maskable Interrupt",
                              "Breakpoint",
                              "Into Detected Overflow",
                              "Out of Bounds",
                              "Invalid Opcode",
                              "No Coprocessor",

                              "Double Fault",
                              "Coprocessor Segment Overrun",
                              "Bad TSS",
                              "Segment Not Present",
                              "Stack Fault",
                              "General Protection Fault",
                              "Page Fault",
                              "Unknown Interrupt",

                              "Coprocessor Fault",
                              "Alignment Check",
                              "Machine Check",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",

                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved"};

void isr_handler(registers_t *r) {
  setTextColor(RED);
  setBackgroundColor(WHITE);
  // kprintf("received interrupt %d\n", r->int_no);
  switch (r->int_no) {
    case 14:
      pageFaultHandler(r);
      break;

    case 13:
      gpFaultHandler(r);
      break;

    default:
      break;
  }

  resetScreenColors();
}

void IRQ_set_mask(unsigned char IRQline) {
  uint16_t port;
  uint8_t value;

  if (IRQline < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    IRQline -= 8;
  }
  value = inb(port) | (1 << IRQline);
  outb(port, value);
}

void IRQ_clear_mask(unsigned char IRQline) {
  uint16_t port;
  uint8_t value;

  if (IRQline < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    IRQline -= 8;
  }
  value = inb(port) & ~(1 << IRQline);
  outb(port, value);
}

void irq_handler(registers_t *r) {
  /* After every interrupt we need to send an EOI to the PICs
   * or they will not send another interrupt again */
  if (r->int_no >= IRQ8) outb(0xA0, 0x20); /* slave */
  outb(0x20, 0x20);                        /* master */

  /* Handle the interrupt in a more modular way */
  if (interrupt_handlers[r->int_no] != 0) {
    isr_t handler = interrupt_handlers[r->int_no];
    handler(r);
  }
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
  interrupt_handlers[n] = handler;
}

void scheduler(registers_t *regs) {
  /*
    Need to reset the register C otherwise no more RTC interrutps will be sent
   */

  // I was in user mode
  uint8_t userMode = FALSE;
  /* if ((regs->cs & 0b11) == 3) userMode = TRUE;

  if (userMode)
    _loadPageDirectory((uint32_t *)PA((uint32_t)kernel_page_directory)); */

  int prevCurOff = getCursorOffset();
  setCursorPos(1, 0);
  kprintf("Kernel Code (%d)\n\n", rand());
  setCursorPos(getOffsetRow(prevCurOff), getOffsetCol(prevCurOff));
  ++tickCount;

  regs->eflags |= 0x200;
  tss.cs = 0x10;

  tss.esp0 = getRegisterValue(ESP);
  if (userMode)
    _loadPageDirectory((uint32_t *)PA((uint32_t)&user_page_directory));
  outb(PIC_CMD_RESET, PORT_PIC_MASTER_CMD);  // select register C
}

void irq_install() {
  asm volatile("cli");
  // Setup requested IRQs
  init_keyboard();
  activateCursor();
  initTimer();
  asm volatile("sti");
}

