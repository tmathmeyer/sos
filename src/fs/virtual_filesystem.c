#include <fs/virtual_filesystem.h>
#include <std/map.h>
#include <std/int.h>
#include <std/string.h>
#include <fs/fs.h>
#include <mem/alloc.h>

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

	filesystem_t *fs;
	int fid = 0;
	F *f = next_available_file(&fid);

	char *maybe_mount = strdup(path);
	int i = 0;
	while(hashmap_get(mounts, maybe_mount, (void **)&fs)) {
		if (maybe_mount[1] == 0) {
			kprintf("could not find mount in %13s\n", path);
			kfree(maybe_mount);
			return 0;
		}
		char *next_attempt = dirname(maybe_mount);
		kfree(maybe_mount);
		maybe_mount = next_attempt;
	}

	int pathlen = strlen(path);
	int mountlen = strlen(maybe_mount);
	kfree(maybe_mount);

	F_err (*OPEN)(F *, char *, uint16_t) = NULL;
	char *PATH;

	if (pathlen == mountlen) {
		PATH = "";
		OPEN = fs->d_open;
	} else {
		PATH = kmalloc(pathlen - mountlen + 1);
		if (!PATH) {
			return 0;
		}
		memcpy(PATH, &path[mountlen-1], pathlen-mountlen);
		PATH[pathlen-mountlen] = 0;
		switch(fs->node_type(PATH)) {
			case FILE:
				OPEN = fs->f_open;
				break;
			case DIRECTORY:
				OPEN = fs->d_open;
				break;
			default:
				return 0;
		}
	}

	if (OPEN(f, PATH, FLAGS)) {
		kfree(PATH);
		return 0;
	}

	f->fs = fs;
	kfree(PATH);
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

int mkdir(char *dir) {
	if (!dir) {
		return 0;
	}

	if (dir[0] != '/') {
		return 0;
	}

	char *parent = dirname(dir);
	if (!parent) {
		return 0;
	}

	char *child = basename(dir);
	if (!child) {
		goto kill_parent;
	}

	int fd = open(parent, 0);
	int ret = 0;
	if (!fd) {
		goto failed;
	}

	if (files[fd].fs->d_mkdir(&files[fd], child)) {
		ret = 1;
	}
	
	close(fd);

failed:
	kfree(child);

kill_parent:
	kfree(parent);
}

int scan_dir(int fd, char **name) {
	if (files[fd].__open__) {
		return files[fd].fs->d_next(&files[fd], name);
	}
	return 1;
}


int countback_to(char *s, int len, char x) {
	int ct = 0;
	while(len && s[len] != x) {
		len--;
		ct++;
	}
	return ct;
}

char *basename(char *path) {
	int s = strlen(path);
	int baselen = countback_to(path, s-1, '/');
	char *result = kcmalloc(sizeof(char), baselen+1);
	if (!result) {
		return NULL;
	}

	memcpy(result, &path[s-baselen], baselen);
	return result;
}

char *dirname(char *path) {
	int s = strlen(path);
	int baselen = countback_to(path, s-1, '/');
	if (baselen == s-1) {
		return strdup("/");
	}
	char *result = kcmalloc(sizeof(char), s-baselen);
	if (!result) {
		return NULL;
	}

	memcpy(result, path, s-baselen-1);
	result[s-baselen-1] = 0;
	return result;
}