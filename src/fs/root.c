#include <fs/root.h>
#include <std/map.h>
#include <std/int.h>
#include <std/string.h>
#include <fs/fs.h>

#include <shell/shell.h>

#define MAX_FILES 256

map_t mounts = NULL;
F files[MAX_FILES];

void root_init() {
	mounts = hashmap_new();
	for (int i=0; i<MAX_FILES; i++) {
		files[i].__open__ = 0;
	}
}

F *next_available_file(int *id) {
	*id = 0;
	for(int i=1; i<MAX_FILES; i++) {
		if (files[i].__open__ == 0) {
			files[i].__open__ = 1;
			*id = i;
			return &files[i];
		}
	}
	return NULL;
}


int mount(char *path, filesystem_t *fs) {
	return hashmap_put(mounts, path, fs);
}

int open(char *path, uint16_t FLAGS) {
	if (path[0] != '/') {
		return 0;
	}

	char **p = split(path, '/');

	filesystem_t *fs;
	int fid = 0;
	F *f = next_available_file(&fid);
	switch(splitlen(p)) {
		case 0:
		case 1:
			goto error;
		case 2:
			if (hashmap_get(mounts, p[1], (void **)&fs)) {
				goto error;
			}
			if (fs->f_open(f, "", FLAGS)) { // TODO make this d_open
				goto error;
			}
			f->fs = fs;
			goto success;
		default:
			if (hashmap_get(mounts, p[1], (void **)&fs)) {
				goto error;
			}
			if (fs->f_open(f, &path[strlen(p[1]) + 1], FLAGS)) {
				goto error;
			}
			f->fs = fs;
			goto success;
	}

error:
	kprintf("failed to open file %04s\n", path);
	split_free(p);
	return 0;

success:
	split_free(p);
	return fid;
}

int read(int fd, void *dest, uint64_t bytes) {
	if (!files[fd].__open__) {
		return 0;
	}

	uint64_t read;
	if (files[fd].fs->f_read(&files[fd], dest, bytes, &read)) {
		return 0;
	}

	return read;
}

int write(int fd, void *src, uint64_t bytes) {
	if (!files[fd].__open__) {
		return 0;
	}
	uint64_t wrote;
	if (files[fd].fs->f_write(&files[fd], src, bytes, &wrote)) {
		return 0;
	}

	return wrote;
}

int close(int fd) {
	if (files[fd].__open__) {
		return files[fd].fs->f_close(&files[fd]);
	}
	return 1;
}

int mkdir(int fd) {

}