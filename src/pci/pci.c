#include <std/int.h>
#include <pci/pci.h>
#include <arch/io.h>
#include <shell/shell.h>

#include "pci_vendor.h"


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

void pci_write_field(uint32_t device, int field, int size, uint32_t value) {
    outl(PCI_ADDRESS_PORT, pci_get_addr(device, field));
    outl(PCI_VALUE_PORT, value);
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
        if (pci_vendor_table[i].ven_id == vendor_id) {
            return pci_vendor_table[i].ven_full;
        }
    }
    return "";
}

pci_devtable_entry_t pci_device_lookup(uint16_t vendor_id, uint16_t dev_id) {
    for(uint64_t i=0; i<PCI_DEVTABLE_LEN; i++) {
        if (pci_device_table[i].ven_id==vendor_id && pci_device_table[i].dev_id==dev_id) {
            return pci_device_table[i];
        }
    }
    pci_devtable_entry_t none = {.ven_id=0, .dev_id=0, .chip="", .chip_desc=""};
    return none;
}

void print_pci_devices(uint32_t device, uint16_t vendorid, uint16_t deviceid, void *na) {
    pci_devtable_entry_t info = pci_device_lookup(vendorid, deviceid);
    kprintf("Device #%03i:\n"
            "   Vendor: %03s\n"
            "   Chip  : %03s\n"
            "   Desc  : %03s\n",
            device,
            pci_vendor_lookup(vendorid),
            info.chip,
            info.chip_desc);
}

void pci_scan_hit(pci_scanner_fn_t fn, uint32_t dev, void *extra) {
    int dev_vend = (int)pci_read_field(dev, PCI_VENDOR_ID, 2);
    int dev_dvid = (int)pci_read_field(dev, PCI_DEVICE_ID, 2);
    fn(dev, dev_vend, dev_dvid, extra);
}

void pci_scan_bus(pci_scanner_fn_t, int type, int bus, void *extra);
void pci_scan_func(pci_scanner_fn_t fn, int type, int bus, int slot, int func, void * extra) {
    uint32_t dev = pci_box_device(bus, slot, func);
    if (type == -1 || type == pci_find_type(dev)) {
        pci_scan_hit(fn, dev, extra);
    }
    if (pci_find_type(dev) == PCI_TYPE_BRIDGE) {
        pci_scan_bus(fn, type, pci_read_field(dev, PCI_SECONDARY_BUS, 1), extra);
    }
}

void pci_scan_slot(pci_scanner_fn_t fn, int type, int bus, int slot, void *extra) {
    uint32_t dev = pci_box_device(bus, slot, 0);
    if (pci_read_field(dev, PCI_VENDOR_ID, 2) == PCI_NONE) {
        return;
    }
    pci_scan_func(fn, type, bus, slot, 0, extra);
    if (!pci_read_field(dev, PCI_HEADER_TYPE, 1)) {
        return;
    }
    for (int func = 1; func < 8; func++) {
        uint32_t dev = pci_box_device(bus, slot, func);
        if (pci_read_field(dev, PCI_VENDOR_ID, 2) != PCI_NONE) {
            pci_scan_func(fn, type, bus, slot, func, extra);
        }
    }
}

void pci_scan_bus(pci_scanner_fn_t fn, int type, int bus, void *extra) {
    for (int slot = 0; slot < 32; ++slot) {
        pci_scan_slot(fn, type, bus, slot, extra);
    }
}

void pci_scan(pci_scanner_fn_t fn, int type, void *extra) {
    if ((pci_read_field(0, PCI_HEADER_TYPE, 1) & 0x80) == 0) {
        pci_scan_bus(fn, type,0,extra);
        return;
    }

    for (int func = 0; func < 8; ++func) {
        uint32_t dev = pci_box_device(0, 0, func);
        if (pci_read_field(dev, PCI_VENDOR_ID, 2) != PCI_NONE) {
            pci_scan_bus(fn, type, func, extra);
        } else {
            break;
        }
    }
}
