#include "pci.h"
#include "ktype.h"
#include "kio.h"
#include "pci_vendor.h"
#include "libk.h"


static inline int pci_extract_bus(uint32_t device) {
    return (uint8_t)((device >> 16));
}
static inline int pci_extract_slot(uint32_t device) {
    return (uint8_t)((device >> 8));
}
static inline int pci_extract_func(uint32_t device) {
    return (uint8_t)(device);
}
static inline uint32_t pci_box_device(int bus, int slot, int func) {
    return (uint32_t)((bus << 16) | (slot << 8) | func);
}

static inline uint32_t pci_get_addr(uint32_t device, int field) {
    return 0x80000000
        | (pci_extract_bus(device) << 16)
        | (pci_extract_slot(device) << 11)
        | (pci_extract_func(device) << 8)
        | ((field) & 0xFC);
}

uint32_t pci_read_field(uint32_t device, int field, int size) {
    outl(PCI_ADDRESS_PORT, pci_get_addr(device, field));

    if (size == 4) {
        uint32_t t = inl(PCI_VALUE_PORT);
        return t;
    } else if (size == 2) {
        uint16_t t = ins(PCI_VALUE_PORT + (field & 2));
        return t;
    } else if (size == 1) {
        uint8_t t = inb(PCI_VALUE_PORT + (field & 3));
        return t;
    }
    return 0xFFFF;
}

uint16_t pci_find_type(uint32_t dev) {
    return (pci_read_field(dev, PCI_CLASS, 1) << 8) | pci_read_field(dev, PCI_SUBCLASS, 1);
}

const char *pci_vendor_lookup(uint16_t vendor_id) {
    for (unsigned int i = 0; i < PCI_VENTABLE_LEN; ++i) {
        if (PciVenTable[i].VenId == vendor_id) {
            return PciVenTable[i].VenFull;
        }
    }
    return "";
}

static uint32_t ata_pci = 0x00000000;
static void find_ata_pci(uint32_t device, uint16_t vendorid, uint16_t deviceid, void * extra) {
    if ((vendorid == 0x8086) && (deviceid == 0x7010 || deviceid == 0x7111)) {
        *((uint32_t *)extra) = device;
    }
}

void pci_scan_hit(uint32_t dev, void * extra) {
    int dev_vend = (int)pci_read_field(dev, PCI_VENDOR_ID, 2);
    int dev_dvid = (int)pci_read_field(dev, PCI_DEVICE_ID, 2);

    kprintf("Device Vendor is %03s\n", pci_vendor_lookup(dev_vend));

    find_ata_pci(dev, dev_vend, dev_dvid, extra);
}

void pci_scan_bus(int type, int bus, void *extra);
void pci_scan_func(int type, int bus, int slot, int func, void * extra) {
    uint32_t dev = pci_box_device(bus, slot, func);
    if (type == -1 || type == pci_find_type(dev)) {
        pci_scan_hit(dev, extra);
    }
    if (pci_find_type(dev) == PCI_TYPE_BRIDGE) {
        pci_scan_bus(type, pci_read_field(dev, PCI_SECONDARY_BUS, 1), extra);
    }
}

void pci_scan_slot(int type, int bus, int slot, void *extra) {
    uint32_t dev = pci_box_device(bus, slot, 0);
    if (pci_read_field(dev, PCI_VENDOR_ID, 2) == PCI_NONE) {
        return;
    }
    pci_scan_func(type, bus, slot, 0, extra);
    if (!pci_read_field(dev, PCI_HEADER_TYPE, 1)) {
        return;
    }
    for (int func = 1; func < 8; func++) {
        uint32_t dev = pci_box_device(bus, slot, func);
        if (pci_read_field(dev, PCI_VENDOR_ID, 2) != PCI_NONE) {
            pci_scan_func(type, bus, slot, func, extra);
        }
    }
}

void pci_scan_bus(int type, int bus, void *extra) {
    for (int slot = 0; slot < 32; ++slot) {
        pci_scan_slot(type, bus, slot, extra);
    }
}

void pci_scan(int type, void *extra) {
    if ((pci_read_field(0, PCI_HEADER_TYPE, 1) & 0x80) == 0) {
        pci_scan_bus(type,0,extra);
        return;
    }
    /*
       for (int func = 0; func < 8; ++func) {
       uint32_t dev = pci_box_device(0, 0, func);
       if (pci_read_field(dev, PCI_VENDOR_ID, 2) != PCI_NONE) {
       pci_scan_bus(f, type, func, extra);
       } else {
       break;
       }
       }
       */
}
