#include <devfs.h>
#include <fs.h>
#include <reefs.h>
#include <vfs.h>
#include <mmu.h>

void devfs_put_device(char *name, struct ata_device *ata) {
	kprintf("storing name %04s\n", name);
}

void devfs_init(void) {
	kprintf("devfs init\n");
	fs_t *fs = new_mem_reefs(4096 * 3);
	mount_fs_by_path(fs, "/tmp");
}

