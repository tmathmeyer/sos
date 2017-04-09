#ifndef pci_h
#define pci_h

#include "ktype.h"

#define PCI_ADDRESS_PORT 0xCF8
#define PCI_VALUE_PORT   0xCFC
#define PCI_NONE 0xFFFF

#define PCI_SUBCLASS             0x0a
#define PCI_CLASS                0x0b
#define PCI_HEADER_TYPE          0x0e
#define PCI_VENDOR_ID            0x00
#define PCI_DEVICE_ID            0x02

#define PCI_HEADER_TYPE_DEVICE  0
#define PCI_HEADER_TYPE_BRIDGE  1
#define PCI_HEADER_TYPE_CARDBUS 2

#define PCI_TYPE_BRIDGE 0x0604
#define PCI_TYPE_SATA   0x0106


#define PCI_SECONDARY_BUS        0x19
typedef void (*pci_scanner_fn_t)(uint32_t, uint16_t, uint16_t, void *);

uint16_t pci_find_type(uint32_t);
const char *pci_vendor_lookup(uint16_t);
void pci_scan(pci_scanner_fn_t, int, void *);
void print_pci_devices(uint32_t, uint16_t, uint16_t, void *);

#endif
