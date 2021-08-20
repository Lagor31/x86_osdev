#include "pci.h"

#include "screen.h"

void checkAllBuses(void) {
  u16 bus;
  u8 device;

  for (bus = 0; bus < 256; bus++) {
    for (device = 0; device < 32; device++) {
      pci_check_vendor(bus, device);
    }
  }
}

u16 pci_config_read_word(u8 bus, u8 slot, u8 func, u8 offset) {
  u32 address;
  u32 lbus = (u32)bus;
  u32 lslot = (u32)slot;
  u32 lfunc = (u32)func;
  u16 tmp = 0;

  /* create configuration address as per Figure 1 */
  address = (u32)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                  (offset & 0xfc) | ((u32)0x80000000));

  /* write out the address */
  outdw(0xCF8, address);
  /* read in the data */
  /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
  tmp = (u32)((indw(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
  return (tmp);
}

u16 pci_check_vendor(u16 bus, u8 slot) {
  uint16_t vendor, device;
  /* try and read the first configuration register. Since there are no */
  /* vendors that == 0xFFFF, it must be a non-existent device. */
  if ((vendor = pci_config_read_word(bus, slot, 0, 0)) != 0xFFFF) {
    device = pci_config_read_word(bus, slot, 0, 2);
    kprintf("Vendor: %x Device %x\n", vendor, device);
  }

  return (vendor);
}