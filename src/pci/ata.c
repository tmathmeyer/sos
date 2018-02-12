#include <pci/ata.h>
#include <pci/pci.h>
#include <pic/interrupts.h>

#include <arch/lock.h>
#include <mem/alloc.h>
#include <mmu/mmu.h>
#include <arch/io.h>

#include <fs/virtual_filesystem.h>
#include <fs/fs.h>
#include <std/string.h>

#include <shell/shell.h>

static struct ata_device ata_primary_master   = {.io_base = 0x1F0, .control = 0x3F6, .slave = 0};
static struct ata_device ata_primary_slave    = {.io_base = 0x1F0, .control = 0x3F6, .slave = 1};
static struct ata_device ata_secondary_master = {.io_base = 0x170, .control = 0x376, .slave = 0};
static struct ata_device ata_secondary_slave  = {.io_base = 0x170, .control = 0x376, .slave = 1};

static char ATA_dev_name[4];
static char ATAPI_dev_name[4];

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

static int ata_status_wait(struct ata_device *dev, int timeout) {
    int status;
    if (timeout > 0) {
        int i = 0;
        while ((status = inb(dev->io_base + ATA_REG_STATUS)) & ATA_SR_BSY && (i < timeout)) i++;
    } else {
        while ((status = inb(dev->io_base + ATA_REG_STATUS)) & ATA_SR_BSY);
    }
    return status;
}

static int ata_wait(struct ata_device *dev, int advanced) {
    uint8_t status = 0;

    ata_io_wait(dev);

    status = ata_status_wait(dev, -1);

    if (advanced) {
        status = inb(dev->io_base + ATA_REG_STATUS);
        if (status   & ATA_SR_ERR)  return 1;
        if (status   & ATA_SR_DF)   return 1;
        if (!(status & ATA_SR_DRQ)) return 1;
    }

    return 0;
}

static bool ata_device_init(struct ata_device *dev, uint32_t ata_pci) {
    outb(dev->io_base + 1, 1);
    outb(dev->control, 0);

    outb(dev->io_base + ATA_REG_HDDEVSEL, 0xA0 | dev->slave << 4);
    ata_io_wait(dev);

    outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_io_wait(dev);

    int status = inb(dev->io_base + ATA_REG_COMMAND);

    ata_wait(dev, 0);

    uint16_t *buf = (uint16_t *)&dev->identity;
    memset(buf, 0, sizeof(ata_identify_t));

    for (int i = 0; i < 256; ++i) {
        buf[i] = ins(dev->io_base);
    }

    uint8_t *ptr = (uint8_t *)&dev->identity.model;
    for (int i = 0; i < 39; i+=2) {
        uint8_t tmp = ptr[i+1];
        ptr[i+1] = ptr[i];
        ptr[i] = tmp;
    }

    dev->is_atapi = 0;

    allocate_full_page_writeback((uint64_t *)&dev->dma_prdt, (uint64_t *)&dev->dma_prdt_phys);
    allocate_full_page_writeback((uint64_t *)&dev->dma_start, (uint64_t *)&dev->dma_start_phys);

    dev->dma_prdt[0].offset = dev->dma_start_phys;
    dev->dma_prdt[0].bytes = 512;
    dev->dma_prdt[0].last = 0x8000;

    uint16_t command_reg = pci_read_field(ata_pci, PCI_COMMAND, 4);
    if (!(command_reg & (1 << 2))) {
        command_reg |= (1 << 2); /* bit 2 */
        pci_write_field(ata_pci, PCI_COMMAND, 4, command_reg);
        command_reg = pci_read_field(ata_pci, PCI_COMMAND, 4);
    }

    dev->bar4 = pci_read_field(ata_pci, PCI_BAR4, 4);

    if (dev->bar4 & 0x00000001) {
        dev->bar4 = dev->bar4 & 0xFFFFFFFC;
    } else {
        kprintf("Error: Disks cannot be setup\n");
        return false; /* No DMA because we're not sure what to do here */
    }
    return true;
}

static bool atapi_device_init(struct ata_device *dev, uint32_t ata_dev) {
    dev->is_atapi = 1;

    outb(dev->io_base + 1, 1);
    outb(dev->control, 0);

    outb(dev->io_base + ATA_REG_HDDEVSEL, 0xA0 | dev->slave << 4);
    ata_io_wait(dev);

    outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
    ata_io_wait(dev);

    int status = inb(dev->io_base + ATA_REG_COMMAND);

    ata_wait(dev, 0);

    uint16_t * buf = (uint16_t *)&dev->identity;

    for (int i = 0; i < 256; ++i) {
        buf[i] = ins(dev->io_base);
    }

    uint8_t * ptr = (uint8_t *)&dev->identity.model;
    for (int i = 0; i < 39; i+=2) {
        uint8_t tmp = ptr[i+1];
        ptr[i+1] = ptr[i];
        ptr[i] = tmp;
    }

    /* Detect medium */
    atapi_command_t command;
    command.command_bytes[0] = 0x25;
    command.command_bytes[1] = 0;
    command.command_bytes[2] = 0;
    command.command_bytes[3] = 0;
    command.command_bytes[4] = 0;
    command.command_bytes[5] = 0;
    command.command_bytes[6] = 0;
    command.command_bytes[7] = 0;
    command.command_bytes[8] = 0; /* bit 0 = PMI (0, last sector) */
    command.command_bytes[9] = 0; /* control */
    command.command_bytes[10] = 0;
    command.command_bytes[11] = 0;

    uint16_t bus = dev->io_base;

    outb(bus + ATA_REG_FEATURES, 0x00);
    outb(bus + ATA_REG_LBA1, 0x08);
    outb(bus + ATA_REG_LBA2, 0x08);
    outb(bus + ATA_REG_COMMAND, ATA_CMD_PACKET);

    /* poll */
    while (1) {
        uint8_t status = inb(dev->io_base + ATA_REG_STATUS);
        if ((status & ATA_SR_ERR)) goto atapi_error;
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) break;
    }

    for (int i = 0; i < 6; ++i) {
        outs(bus, command.command_words[i]);
    }

    /* poll */
    while (1) {
        uint8_t status = inb(dev->io_base + ATA_REG_STATUS);
        if ((status & ATA_SR_ERR)) goto atapi_error_read;
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) break;
        if ((status & ATA_SR_DRQ)) break;
    }

    uint16_t data[4];

    for (int i = 0; i < 4; ++i) {
        data[i] = ins(bus);
    }

#define htonl(l)  ( (((l) & 0xFF) << 24) | (((l) & 0xFF00) << 8) | (((l) & 0xFF0000) >> 8) | (((l) & 0xFF000000) >> 24))
    uint32_t lba, blocks;;
    memcpy(&lba, &data[0], sizeof(uint32_t));
    lba = htonl(lba);
    memcpy(&blocks, &data[2], sizeof(uint32_t));
    blocks = htonl(blocks);

    dev->atapi_lba = lba;
    dev->atapi_sector_size = blocks;

    kprintf("Finished! LBA = %03x; block length = %03x\n", lba, blocks);
    return true;

atapi_error_read:
    kprintf("ATAPI error; no medium (disk player empty?)\n");
    return false;

atapi_error:
    kprintf("ATAPI early error; unsure\n");
    return false;

}

uint64_t ata_max_offset(struct ata_device *dev) {
    uint64_t sectors = dev->identity.sectors_48;
    if (!sectors) {
        /* Fall back to sectors_28 */
        sectors = dev->identity.sectors_28;
    }

    return sectors * ATA_SECTOR_SIZE;
}

lock_t ata_lock;
void ata_device_read_sector(struct ata_device *dev, uint32_t lba, uint8_t *buf) {
    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    if (dev->is_atapi) return;

    spin_lock(&ata_lock);

    ata_wait(dev, 0);

    /* Stop */
    outb(dev->bar4, 0x00);

    /* Set the PRDT */
    outl(dev->bar4 + 0x04, (uint32_t)(uint64_t)dev->dma_prdt_phys);

    /* Enable error, irq status */
    outb(dev->bar4 + 0x2, inb(dev->bar4 + 0x02) | 0x04 | 0x02);

    /* set read */
    outb(dev->bar4, 0x08);

    //IRQ_ON;
    while (1) {
        uint8_t status = inb(dev->io_base + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) break;
    }

    outb(bus + ATA_REG_CONTROL, 0x00);
    outb(bus + ATA_REG_HDDEVSEL, 0xe0 | slave << 4 | (lba & 0x0f000000) >> 24);
    ata_io_wait(dev);
    outb(bus + ATA_REG_FEATURES, 0x00);
    outb(bus + ATA_REG_SECCOUNT0, 1);
    outb(bus + ATA_REG_LBA0, (lba & 0x000000ff) >>  0);
    outb(bus + ATA_REG_LBA1, (lba & 0x0000ff00) >>  8);
    outb(bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
    while (1) {
        uint8_t status = inb(dev->io_base + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) break;
    }
    outb(bus + ATA_REG_COMMAND, ATA_CMD_READ_DMA);

    ata_io_wait(dev);

    outb(dev->bar4, 0x08 | 0x01);

    while (1) {
        int status = inb(dev->bar4 + 0x02);
        int dstatus = inb(dev->io_base + ATA_REG_STATUS);
        if (!(status & 0x04)) {
            continue;
        }
        if (!(dstatus & ATA_SR_BSY)) {
            break;
        }
    }
    //IRQ_OFF;

    /* Copy from DMA buffer to output buffer. */
    memcpy(buf, dev->dma_start, 512);

    /* Inform device we are done. */
    outb(dev->bar4 + 0x2, inb(dev->bar4 + 0x02) | 0x04 | 0x02);
    spin_unlock(&ata_lock);
}

void ata_device_write_sector(struct ata_device *dev, uint32_t lba, uint8_t *buf) {
    uint16_t bus = dev->io_base;
    uint8_t slave = dev->slave;

    spin_lock(&ata_lock);

    outb(bus + ATA_REG_CONTROL, 0x02);

    ata_wait(dev, 0);
    outb(bus + ATA_REG_HDDEVSEL, 0xe0 | slave << 4 | (lba & 0x0f000000) >> 24);
    ata_wait(dev, 0);

    outb(bus + ATA_REG_FEATURES, 0x00);
    outb(bus + ATA_REG_SECCOUNT0, 0x01);
    outb(bus + ATA_REG_LBA0, (lba & 0x000000ff) >>  0);
    outb(bus + ATA_REG_LBA1, (lba & 0x0000ff00) >>  8);
    outb(bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
    outb(bus + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    ata_wait(dev, 0);
    int size = ATA_SECTOR_SIZE / 2;
    outsm(bus,buf,size);
    outb(bus + 0x07, ATA_CMD_CACHE_FLUSH);
    ata_wait(dev, 0);
    spin_unlock(&ata_lock);
}

static int buffer_compare(uint32_t * ptr1, uint32_t * ptr2, uint64_t size) {
    if (size % 4) {
        ERROR("tried to a buffer compare with invalid size");
        while(1);
    }
    uint64_t i = 0;
    while (i < size) {
        if (*ptr1 != *ptr2) return 1;
        ptr1++;
        ptr2++;
        i += sizeof(uint32_t);
    }
    return 0;
}

void ata_device_write_sector_retry(struct ata_device *dev, uint32_t lba, uint8_t *buf) {
    char read_buf[ATA_SECTOR_SIZE]; /* TODO: put this on the heap! */
    do {
        ata_device_write_sector(dev, lba, buf);
        ata_device_read_sector(dev, lba, read_buf);
    } while (buffer_compare((uint32_t *)buf, (uint32_t *)read_buf, ATA_SECTOR_SIZE));
}

static uint32_t write_ata(struct ata_device *dev, uint32_t offset, uint32_t size, uint8_t *buffer) {
    unsigned int start_block = offset / ATA_SECTOR_SIZE;
    unsigned int end_block = (offset + size - 1) / ATA_SECTOR_SIZE;

    unsigned int x_offset = 0;

    if (offset > ata_max_offset(dev)) {
        return 0;
    }

    if (offset + size > ata_max_offset(dev)) {
        unsigned int i = ata_max_offset(dev) - offset;
        size = i;
    }

    if (offset % ATA_SECTOR_SIZE) {
        unsigned int prefix_size = (ATA_SECTOR_SIZE - (offset % ATA_SECTOR_SIZE));

        char tmp[ATA_SECTOR_SIZE]; /* TODO: put this on the heap! */
        ata_device_read_sector(dev, start_block, (uint8_t *)tmp);

        memcpy((void *)((uintptr_t)tmp + (offset % ATA_SECTOR_SIZE)), buffer, prefix_size);
        ata_device_write_sector_retry(dev, start_block, (uint8_t *)tmp);

        x_offset += prefix_size;
        start_block++;
    }

    if ((offset + size)  % ATA_SECTOR_SIZE && start_block <= end_block) {
        unsigned int postfix_size = (offset + size) % ATA_SECTOR_SIZE;

        char tmp[ATA_SECTOR_SIZE]; /* TODO: put this on the heap! */
        ata_device_read_sector(dev, end_block, (uint8_t *)tmp);

        memcpy(tmp, (void *)((uintptr_t)buffer + size - postfix_size), postfix_size);

        ata_device_write_sector_retry(dev, end_block, (uint8_t *)tmp);

        end_block--;
    }

    while (start_block <= end_block) {
        ata_device_write_sector_retry(dev, start_block, (uint8_t *)((uintptr_t)buffer + x_offset));
        x_offset += ATA_SECTOR_SIZE;
        start_block++;
    }

    return size;
}

static uint32_t read_ata(struct ata_device *dev, uint32_t offset, uint32_t size, uint8_t *buffer) {
    unsigned int start_block = offset / ATA_SECTOR_SIZE;
    unsigned int end_block = (offset + size - 1) / ATA_SECTOR_SIZE;

    unsigned int x_offset = 0;

    if (offset > ata_max_offset(dev)) {
        return 0;
    }

    if (offset + size > ata_max_offset(dev)) {
        unsigned int i = ata_max_offset(dev) - offset;
        size = i;
    }


    if (offset % ATA_SECTOR_SIZE) {
        unsigned int prefix_size = (ATA_SECTOR_SIZE - (offset % ATA_SECTOR_SIZE));
        char tmp[ATA_SECTOR_SIZE]; /* TODO: put this on the heap! */
        ata_device_read_sector(dev, start_block, (uint8_t *)tmp);

        memcpy(buffer, (void *)((uintptr_t)tmp + (offset % ATA_SECTOR_SIZE)), prefix_size);

        x_offset += prefix_size;
        start_block++;
    }

    if ((offset + size)  % ATA_SECTOR_SIZE && start_block <= end_block) {
        unsigned int postfix_size = (offset + size) % ATA_SECTOR_SIZE;
        char tmp[ATA_SECTOR_SIZE]; /* TODO: put this on the heap! */
        ata_device_read_sector(dev, end_block, (uint8_t *)tmp);
        memcpy((void *)((uintptr_t)buffer + size - postfix_size), tmp, postfix_size);
        end_block--;
    }

    while (start_block <= end_block && end_block != 0xFFFFFFFF) {
        ata_device_read_sector(dev, start_block, (uint8_t *)((uintptr_t)buffer + x_offset));
        x_offset += ATA_SECTOR_SIZE;
        start_block++;
    }

    return size;
}

uint64_t ata_block_read(block_device_t *b, uint64_t addr, uint64_t chars, uint8_t *data) {
    return read_ata(b->__private__, addr, chars, data);
}

uint64_t ata_block_write(block_device_t *b, uint64_t addr, uint64_t chars, uint8_t *data) {
    return write_ata(b->__private__, addr, chars, data);
}

void write_block_device_from_ata(char *path, struct ata_device *dev) {
    block_device_t device;
    device.__private__ = path;
    device.write = ata_block_write;
    device.read = ata_block_read;

    int fd = open(path, CREATE_ON_OPEN | CREATE_BLOCK_DEVICE);
    if (!fd) {
        return;
    }
    write(fd, &device, sizeof(block_device_t));
    close(fd);
}

static int ata_device_detect(struct ata_device *dev, uint32_t ata_dev) {
    ata_soft_reset(dev);
    ata_io_wait(dev);
    outb(dev->io_base + ATA_REG_HDDEVSEL, 0xA0 | dev->slave << 4);
    ata_io_wait(dev);
    ata_status_wait(dev, 10000);

    unsigned char cl = inb(dev->io_base + ATA_REG_LBA1); /* CYL_LO */
    unsigned char ch = inb(dev->io_base + ATA_REG_LBA2); /* CYL_HI */

    if (cl == 0xFF && ch == 0xFF) {
        return 0;
    }
    if ((cl == 0x00 && ch == 0x00) || (cl == 0x3C && ch == 0xC3)) {
        if (ata_device_init(dev, ata_dev)) {
            char *e = strcat("/ata/", ATA_dev_name);
            write_block_device_from_ata(e, dev);
            kfree(e);
            ATA_dev_name[2]++;
        }
        return 1;
    } else if ((cl == 0x14 && ch == 0xEB) ||
            (cl == 0x69 && ch == 0x96)) {
        if (atapi_device_init(dev, ata_dev)) {
            char *e = strcat("/ata/", ATA_dev_name);
            write_block_device_from_ata(e, dev);
            kfree(e);
            ATAPI_dev_name[2]++;
        }
        return 2;
    }
    return 0;
}

uint64_t read_disk_raw(struct ata_device *dev, uint64_t addr, uint64_t chars, uint8_t *data) {
    return read_ata(dev, addr, chars, data);
}

uint64_t write_disk_raw(struct ata_device *dev, uint64_t addr, uint64_t chars, uint8_t *data) {
    return write_ata(dev, addr, chars, data);
}

void ata_init() {
    mkdir("/ata");
    uint32_t ata_dev = 0;
    memcpy(ATA_dev_name, "hda", 4);
    memcpy(ATAPI_dev_name, "eda", 4);
    pci_scan(&find_ata_pci, -1, &ata_dev);

    set_interrupt_handler(0x2E, ata_irq);
    set_interrupt_handler(0x2F, ata_irq);

    ata_device_detect(&ata_primary_master, ata_dev);
    ata_device_detect(&ata_primary_slave, ata_dev);
    ata_device_detect(&ata_secondary_master, ata_dev);
    ata_device_detect(&ata_secondary_slave, ata_dev);
}
