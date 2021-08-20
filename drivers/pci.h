#ifndef PCI_H
#define PCI_H

#include "../cpu/ports.h"
#include "../cpu/types.h"

void checkAllBuses(void);
u16 pci_check_vendor(u16 bus, u8 slot);
u16 pci_config_read_word(u8 bus, u8 slot, u8 func, u8 offset);
#endif