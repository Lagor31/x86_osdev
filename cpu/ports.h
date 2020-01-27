#ifndef PORTS_H
#define PORTS_H

/* Screen i/o ports */
#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

unsigned char inb (unsigned short port);
void outb (unsigned short port, unsigned char data);
unsigned short inw (unsigned short port);
void outw (unsigned short port, unsigned short data);


#endif