#ifndef ata_h
#define ata_h

#include <ktype.h>

#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_STATUS     0x07

#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DF      0x20
#define ATA_SR_DSC     0x10
#define ATA_SR_DRQ     0x08
#define ATA_SR_CORR    0x04
#define ATA_SR_IDX     0x02
#define ATA_SR_ERR     0x01

#define ATA_SECTOR_SIZE 512

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

typedef struct {
    uint32_t *offset;
    uint16_t bytes;
    uint16_t last;
} prdt_t;

typedef union {
    uint8_t command_bytes[12];
    uint16_t command_words[6];
} atapi_command_t;

typedef struct {
    uint16_t flags;
    uint16_t unused1[9];
    char     serial[20];
    uint16_t unused2[3];
    char     firmware[8];
    char     model[40];
    uint16_t sectors_per_int;
    uint16_t unused3;
    uint16_t capabilities[2];
    uint16_t unused4[2];
    uint16_t valid_ext_data;
    uint16_t unused5[5];
    uint16_t size_of_rw_mult;
    uint32_t sectors_28;
    uint16_t unused6[38];
    uint64_t sectors_48;
    uint16_t unused7[152];
} __attribute__((packed)) ata_identify_t;

struct ata_device {
    int io_base;
    int control;
    int slave;
    int is_atapi;
    ata_identify_t identity;
    prdt_t *dma_prdt;
    uint32_t *dma_prdt_phys;
    uint8_t *dma_start;
    uint32_t *dma_start_phys;
    uint32_t bar4;
    uint32_t atapi_lba;
    uint32_t atapi_sector_size;
};

void ata_init();
uint64_t read_disk_raw(uint64_t, uint64_t, uint8_t *);
uint64_t write_disk_raw(uint64_t, uint64_t, uint8_t *);

void ata_device_read_sector(struct ata_device *, uint32_t, uint8_t *);
void ata_device_write_sector(struct ata_device *, uint32_t, uint8_t *);
void ata_device_write_sector_retry(struct ata_device *, uint32_t, uint8_t *);

uint64_t ata_max_offset(struct ata_device *);

#endif
