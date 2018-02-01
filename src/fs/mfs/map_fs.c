#include <fs/mfs/map_fs.h>
#include <fs/fs.h>
#include <mem/alloc.h>
#include <std/map.h>
#include <std/int.h>
#include <std/string.h>

map_t root;
filesystem_t *filesystem;

F_err mfs_open(F *file, char *name, uint16_t mode) {
	if (!file) {
		return FILE_NOT_FOUND;
	}
	map_t dir = root;
	d_f *dir_or_file;
	char **path = split(name, '/');
	for(int i=1; i<splitlen(path); i++) {
		if (hashmap_get(dir, path[i-1], (void **)&dir_or_file)) {
			goto err;
		}
		if (dir_or_file->type == FILE) {
			goto err;
		}
		dir = dir_or_file->dir;
	}
	if (hashmap_get(dir, path[splitlen(path)-1], (void **)&dir_or_file)) {
		dir_or_file = kmalloc(sizeof(d_f));
		dir_or_file->type = FILE;
		dir_or_file->data = NULL;
		dir_or_file->size = 0;
		if (hashmap_put(dir, path[splitlen(path)-1], dir_or_file)) {
			kfree(dir_or_file);
			goto err;
		}
	}
	if (dir_or_file->type == DIR) {
		goto err;
	}
	split_free(path);
	file->__position__ = 0;
	file->__open__ = true;
	file->__data__ = dir_or_file;
	file->fs = filesystem;
	return NO_ERROR;

err:
	split_free(path);
	return FILE_NOT_FOUND;
}

F_err mfs_close(F *file) {
	file->fs = NULL;
	file->__open__ = false;
	file->__data__ = NULL;
	file->__position__ = 0;
}

F_err mfs_read(F *file, void *dest, uint64_t to_read, uint64_t *actually_read) {
	if (!actually_read) {
		goto badargs;
	}
	*actually_read = 0;
	
	if (!file->__open__) {
		goto badargs;
	}
	if (!dest) {
		goto badargs;
	}
	if (!to_read) {
		goto badargs;
	}

	d_f *m_entry = (d_f *)file->__data__;
	char *data = m_entry->data;
	char *v = dest;
	while(file->__position__ + *actually_read < m_entry->size && to_read) {
		to_read --;
		v[*actually_read] = data[*actually_read + file->__position__];
		actually_read[0]++;
	}

	file->__position__ += *actually_read;
	return NO_ERROR;

badargs:
	return ARGUMENT_ERROR;
}

F_err mfs_write(F *file, void *src, uint64_t to_write, uint64_t *actually_wrote) {
	if (!actually_wrote) {
		goto badargs;
	}
	*actually_wrote = 0;
	
	if (!file->__open__) {
		goto badargs;
	}
	if (!src) {
		goto badargs;
	}
	if (!to_write) {
		goto badargs;
	}

	d_f *fdata = (d_f *)file->__data__;
	uint64_t newsize = file->__position__ + to_write;

	if (newsize > fdata->size) {
		char *newdata = kmalloc(sizeof(char *) * newsize);
		if (fdata->data) {
			memcpy(newdata, fdata->data, file->__position__);
			kfree(fdata->data);
		}
		fdata->data = newdata;
		fdata->size = newsize;
	}

	char *offset = &fdata->data[file->__position__];
	memcpy(offset, src, to_write);
	*actually_wrote = to_write;

	return NO_ERROR;

badargs:
	return ARGUMENT_ERROR;
}


filesystem_t *mfs_new_fs() {
	filesystem = kmalloc(sizeof(filesystem_t));
	root = hashmap_new();
	return filesystem;
}