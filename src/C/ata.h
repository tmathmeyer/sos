#ifndef ata_h
#define ata_h

#define ATA_REG_ALTSTATUS  0x0C

struct ata_device {
    int io_base;
    int control;
    int slave;
};

void ata_init();

#endif
