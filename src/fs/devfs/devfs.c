#include <devfs.h>
#include <mmu.h>

void devfs_put_device(char *name, struct ata_device *ata) {
	kprintf("storing name %04s\n", name);
}

void devfs_init(void) {
	kprintf("devfs init\n");
}

