#include <filesystem.h>
#include <mmu.h>
#include <libk.h>
#include <kio.h>
#include <devices.h>

typedef struct {
	char name[120];
	fs_t *fs;
}__attribute__((packed)) mp_t;

fs_t *maybe_mount_point(mp_t *mp_page, char *path) {
	for(int i=0; i<PAGE_SIZE/sizeof(mp_t); i++) {
		if (mp_page[i].fs && !strncmp(path, mp_page[i].name, 120)) {
			return mp_page[i].fs;
		}
	}
	return NULL;
}

fs_t *get_mount_for_file(mp_t *mp_page, char *path, char **rest) {
	uint64_t len = strlen(path);
	while(len) {
		fs_t *res = maybe_mount_point(mp_page, path);
		if (res) {
			return res;
		}
		while(len && path[len] != '/') {
			len --;
		}
		if (!len) {
			return NULL;
		}
		*rest = &path[len+1];
		path[len] = 0;
	} 
}

FS_ERROR file_read(fs_t *vfs, char *name, void *data, uint64_t len, uint64_t *read) {
	char *innerfile;
	fs_t *fs = get_mount_for_file(vfs->_underlying_data, name, &innerfile);
	kprintf("device read fs = %05x\n", fs);
	kprintf("inner path = \"%04s\"\n", innerfile);
	return fs->file_read(fs, innerfile, data, len, read);
}

FS_ERROR file_write(fs_t *vfs, char *name, void *data, uint64_t len) {
	char *innerfile;
	fs_t *fs = get_mount_for_file(vfs->_underlying_data, name, &innerfile);
	return fs->file_write(fs, innerfile, data, len);
}

FS_ERROR file_delete(fs_t *vfs, char *name) {
	char *innerfile;
	fs_t *fs = get_mount_for_file(vfs->_underlying_data, name, &innerfile);
	return fs->file_delete(fs, innerfile);
}

bool file_exists(fs_t *vfs, char *name) {
	char *innerfile;
	fs_t *fs = get_mount_for_file(vfs->_underlying_data, name, &innerfile);
	return fs->file_exists(fs, innerfile);
}

bool VFS_identity(char *name) {
	return !strncmp(name, "VFS", 4);
}

uint64_t no_size(fs_t *fs) {
	return 0;
}

void fs_init(fs_t *rootfs) {
	fs_t VFS;
	VFS.file_read = file_read;
	VFS.file_write = file_write;
	VFS.file_delete = file_delete;
	VFS.file_exists = file_exists;
	VFS.identity = VFS_identity;
	VFS.get_fs_size_bytes = no_size;
	mp_t *mounts = (void *)starting_address(allocate_full_page());
	memset(mounts, 0, sizeof(PAGE_SIZE));
	mounts[0].fs = rootfs;
	memcpy(mounts[0].name, "/", 1);
	VFS._underlying_data = mounts;
	put_device("VFS", VFS);
}

/* mount a filesystem */
void mount_fs(fs_t *entry, char *location) {
	fs_t *VFS = get_device("VFS");
	if (!VFS->file_exists(VFS, location)) {
		return;
	}
	if (strlen(location) > 120) {
		return;
	}
	mp_t *mounts = VFS->_underlying_data;
	for(int i=0; i<PAGE_SIZE/sizeof(mp_t); i++) {
		if (!mounts[i].fs) {
			mounts[i].fs = entry;
			memcpy(mounts[i].name, location, strlen(location));
		}
	}
}

/* unmount a filesystem */
void unmount_fs(char *location) {
	mp_t *mounts = get_device("VFS")->_underlying_data;
	uint64_t len = strlen(location);
	if (len > 120) {
		return;
	}
	for(int i=0; i<PAGE_SIZE/sizeof(mp_t); i++) {
		if (mounts[i].fs && !strncmp(location, mounts[i].name, 120)) {
			for(int j=0; j<PAGE_SIZE/sizeof(mp_t); j++) {
				if (mounts[j].fs && !strncmp(location, mounts[i].name, len)) {
					mounts[j].fs = NULL;
				}
			}
		}
	}
}