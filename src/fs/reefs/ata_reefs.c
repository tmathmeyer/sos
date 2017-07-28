#include <fs.h>
#include <alloc.h>
#include <ktype.h>
#include <reefs.h>
#include <ata.h>
#include <libk.h>

struct ata_device *get_device(fs_disk_t *disk, struct ata_device **dev) {
    memcpy(dev, disk->_data, sizeof(void *));
}

uint64_t ata_write_offset(fs_disk_t *disk, uint64_t offset, void *src, uint64_t len) {
    struct ata_device *dev;
    get_device(disk, &dev);
    return write_disk_raw(dev, offset, len, src);
}

uint64_t ata_read_offset(fs_disk_t *disk, uint64_t offset, void *dest, uint64_t len) {
    struct ata_device *dev;
    get_device(disk, &dev);
    return read_disk_raw(dev, offset, len, dest);
}
	
uint64_t ata_get_size(fs_disk_t *disk) {
    struct ata_device *dev;
    get_device(disk, &dev);
    return ata_max_offset(dev);
}

void new_ata_reefs(struct ata_device *dev, fs_t *fs) {
	fs_disk_t *disk = kmalloc(sizeof(fs_disk_t));

    fs_t __fs = empty_fs();
	fs_disk_t __disk = {
		.read_offset = ata_read_offset,
		.write_offset = ata_write_offset,
		.get_size = ata_get_size
	};

	memcpy(fs, &__fs, sizeof(fs_t));
	memcpy(disk, &__disk, sizeof(fs_disk_t));
    
    memcpy(disk->_data, &dev, sizeof(void *));

	fs->mkfs(fs, disk);
    kprintf("(%03x) statptr = [%05x], stat = %05x\n", fs, &(fs->stat), fs->stat);
	return fs;
}


