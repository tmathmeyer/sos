#include "ata.h"
#include "pci.h"
#include "interrupts.h"
#include "libk.h"
#include "kio.h"

static struct ata_device ata_primary_master = {.io_base = 0x1F0, .control = 0x3F6, .slave = 0};


void find_ata_pci(uint32_t device, uint16_t vendorid, uint16_t deviceid, void * extra) {
    if ((vendorid == 0x8086) && (deviceid == 0x7010 || deviceid == 0x7111)) {
        *((uint32_t *)extra) = device;
    }
}

void ata_irq(struct interrupt_frame *frame) {
    kprintf("ATA irq");
}

static void ata_io_wait(struct ata_device *dev) {
    inb(dev->io_base + ATA_REG_ALTSTATUS);
    inb(dev->io_base + ATA_REG_ALTSTATUS);
    inb(dev->io_base + ATA_REG_ALTSTATUS);
    inb(dev->io_base + ATA_REG_ALTSTATUS);
}

static void ata_soft_reset(struct ata_device *dev) {
    outb(dev->control, 0x04);
    ata_io_wait(dev);
    outb(dev->control, 0x00);
}

void ata_init() {
    uint32_t ata_dev = 0;
    pci_scan(&find_ata_pci, -1, &ata_dev);
    kprintf("ATA device ID: %03x\n", ata_dev);

    set_interrupt_handler(0x2E, ata_irq);
    set_interrupt_handler(0x2F, ata_irq);

    ata_soft_reset(&ata_primary_master);
}
