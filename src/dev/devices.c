#include "devices.h"
#include "shittyfs.h"
#include "mmu.h"
#include "libk.h"
#include "kio.h"

#define NAMELEN 63-sizeof(fs_t)

typedef struct {
	uint8_t present;
	char name[NAMELEN];
	fs_t filesystem;
}__attribute__((packed)) dev_entry_t;

/* requires memory allocator to be present! */
static dev_entry_t *entries = NULL;
void dev_init() {
	if (entries == NULL) {
		entries = (void *)starting_address(allocate_full_page());
		memset(entries, 0, PAGE_SIZE);
	}
}

fs_t *get_device(char *name) {
	for(size_t i=0; i<PAGE_SIZE/sizeof(dev_entry_t); i++) {
		if (entries[i].present) {
			if (!strncmp(entries[i].name, name, min(strlen(name), NAMELEN))) {
				return &(entries[i].filesystem);
			}
		}
	}
	return NULL;
}

void put_device(char *name, fs_t device) {
	for(size_t i=0; i<PAGE_SIZE/sizeof(dev_entry_t); i++) {
		if (!entries[i].present) {
			entries[i].present = 1;
			entries[i].filesystem = device;
			memcpy(entries[i].name, name, min(strlen(name), NAMELEN));
			return;
		}
	}
	kprintf("No file entries remaining\n");
}

void list_devices() {
	for(size_t i=0; i<PAGE_SIZE/sizeof(dev_entry_t); i++) {
		if (entries[i].present) {
			char buf[NAMELEN+1] = {0};
			memcpy(buf, entries[i].name, NAMELEN);
			kprintf("device: %04s\n", buf);
		}
	}
}

void each_device(void (*each)(fs_t *, char *)) {
	for(size_t i=0; i<PAGE_SIZE/sizeof(dev_entry_t); i++) {
		if (entries[i].present) {
			char buf[NAMELEN+1] = {0};
			memcpy(buf, entries[i].name, NAMELEN);
			each(&entries[i].filesystem, buf);
		}
	}
}
