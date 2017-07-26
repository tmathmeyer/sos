#include <fs.h>
#include <alloc.h>
#include <ktype.h>
#include <reefs.h>

#include <libk.h>

void fill_mem_disk(fs_disk_t *disk, uint64_t size) {
	struct {
		void *mem;
		uint64_t size;
	} copy = {
		.mem = kmalloc(size),
		.size = size
	};
	memcpy(disk->_data, &copy, sizeof(copy));
}

uint64_t mem_write_offset(fs_disk_t *disk, uint64_t offset, void *src, uint64_t len) {
	struct {
		void *mem;
		uint64_t size;
	} copy;
	memcpy(&copy, disk->_data, sizeof(copy));

	uint8_t *ptr = copy.mem;

	memcpy(ptr+offset, src, len);

	return len;
}

uint64_t mem_read_offset(fs_disk_t *disk, uint64_t offset, void *dest, uint64_t len) {
	struct {
		void *mem;
		uint64_t size;
	} copy;
	memcpy(&copy, disk->_data, sizeof(copy));

	uint8_t *ptr = copy.mem;

	memcpy(dest, ptr+offset, len);

	return len;
}
	
/* get size of the disk IN BYTES */
uint64_t mem_get_size(fs_disk_t *disk) {
	struct {
		void *mem;
		uint64_t size;
	} copy;
	memcpy(&copy, disk->_data, sizeof(copy));
	return copy.size / 512;
}

fs_t *new_mem_reefs(uint64_t size) {
	fs_t *fs = kmalloc(sizeof(fs_t));
	fs_disk_t *disk = kmalloc(sizeof(fs_disk_t));

	fs_t __fs = empty_fs();
	fs_disk_t __disk = {
		.read_offset = mem_read_offset,
		.write_offset = mem_write_offset,
		.get_size = mem_get_size
	};

	memcpy(fs, &__fs, sizeof(fs_t));
	memcpy(disk, &__disk, sizeof(fs_disk_t));

	fill_mem_disk(disk, size);
	fs->mkfs(fs, disk);
	return fs;
}