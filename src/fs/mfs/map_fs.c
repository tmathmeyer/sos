#include <fs/mfs/map_fs.h>
#include <fs/fs.h>
#include <mem/alloc.h>
#include <std/map.h>
#include <std/int.h>
#include <std/string.h>
#include <arch/misc.h>

#include <shell/shell.h>

map_t root;
filesystem_t *filesystem;
d_f __root__;

filesystem_t *mfs_new_fs() {
	filesystem = kmalloc(sizeof(filesystem_t));
	root = hashmap_new();
	__root__.dir = root;
	__root__.other = NULL;
	__root__.size = 0;
	__root__.type = MAP_DIR;
	return filesystem;
}

F_type mfs_node_type(char *name) {
	if (!name) {
		return INVALID;
	}
	map_t dir = root;
	d_f *dir_or_file = &__root__;
	int entries = strseg_count(name, '/');

	for(int i=0; i<entries; i++) {
		char *entry = strseg(name, '/', i);
		if (!entry) {
			goto out;
		}

		if (hashmap_get(dir, entry, (void **)&dir_or_file)) {
			kfree(entry);
			goto out;
		}

		kfree(entry);
		dir = dir_or_file->dir;
	}

out:
	/* NOTE: this does not support symlinks */
	switch(dir_or_file->type) {
		case MAP_DIR:
			return DIRECTORY;
		case MAP_FILE:
			return FILE;
		case MAP_BLOCK:
			return BLOCK_DEVICE;
		default:
			return INVALID;
	}
}

F_err mfs_f_open(F *file, char *name, uint16_t mode) {
	if (!file) {
		return FILE_NOT_FOUND;
	}
	map_t dir = root;
	d_f *dir_or_file = NULL;
	int entries = strseg_count(name, '/');

	for(int i=0; i<entries-1; i++) {
		char *entry = strseg(name, '/', i);

		if (hashmap_get(dir, entry, (void **)&dir_or_file)) {
			kfree(entry);
			goto err;
		}
		kfree(entry);

		if (dir_or_file->type == MAP_FILE) {
			goto err;
		}

		dir = dir_or_file->dir;
	}

	char *entry = strseg(name, '/', entries-1);
	if (hashmap_get(dir, entry, (void **)&dir_or_file)) {
		dir_or_file = kmalloc(sizeof(d_f));
		if (!dir_or_file) {
			kfree(entry);
			goto err;
		}

		if (mode & CREATE_BLOCK_DEVICE) {
			dir_or_file->type = MAP_BLOCK;
		} else {
			dir_or_file->type = MAP_FILE;
		}
		dir_or_file->data = NULL;
		dir_or_file->other = NULL;
		dir_or_file->size = 0;
		dir_or_file->reflock = 0;
		if (hashmap_put(dir, entry, dir_or_file)) {
			kfree(entry);
			kfree(dir_or_file);
			goto err;
		}
	}
	kfree(entry);

	if (dir_or_file->type == MAP_DIR) {
		goto err;
	}

	spin_lock(&dir_or_file->reflock);
	ref_count_inc(&dir_or_file->refcount);
	spin_unlock(&dir_or_file->reflock);


	file->__position__ = 0;
	file->__open__ = true;
	file->__data__ = dir_or_file;
	switch(dir_or_file->type) {
		case MAP_FILE:
			file->__type__ = FILE;
			break;
		case MAP_BLOCK:
			file->__type__ = BLOCK_DEVICE;
			break;
		default:
			kprintf("kernel filesystem open failed\n");
			while(1);
	}
	file->fs = filesystem;
	return NO_ERROR;

err:
	return FILE_NOT_FOUND;
}

F_err mfs_f_close(F *file) {
	d_f *k = file->__data__;

	file->fs = NULL;
	file->__open__ = false;
	file->__data__ = NULL;
	file->__position__ = 0;

	spin_lock(&k->reflock);
	ref_count_dec(&k->refcount);
	spin_unlock(&k->reflock);
}


#define FAIL_ON(condition) do {if((condition))return ARGUMENT_ERROR;}while(0)



F_err mfs_f_read(F *file, void *dest, uint64_t to_read, uint64_t *actually_read) {
	FAIL_ON(!file);
	FAIL_ON(!actually_read);
	FAIL_ON(!file->__open__);
	FAIL_ON(!dest);
	FAIL_ON(!to_read);
	FAIL_ON(file->__type__ != FILE);

	*actually_read = 0;
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
}

F_err mfs_f_write(F *file, void *src, uint64_t to_write, uint64_t *actually_wrote) {
	FAIL_ON(!file);
	FAIL_ON(!actually_wrote);
	FAIL_ON(!file->__open__);
	FAIL_ON(!src);
	FAIL_ON(!to_write);
	FAIL_ON(file->__type__ != FILE && file->__type__ != BLOCK_DEVICE);
	*actually_wrote = 0;

	d_f *fdata = (d_f *)file->__data__;
	uint64_t newsize = file->__position__ + to_write;

	if (newsize > fdata->size) {
		char *newdata = kmalloc(sizeof(char *) * newsize);
		if (!newdata) {
			return FILESYSTEM_ERROR;
		}
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
}

F_err mfs_f_lseek(F *file, uint64_t seek_to) {
	FAIL_ON(!file);
	FAIL_ON(!file->__open__);
	FAIL_ON(file->__type__ != FILE);

	d_f *fdata = (d_f *)file->__data__;
	FAIL_ON(fdata->size < seek_to);

	file->__position__ = seek_to;
	return NO_ERROR;
}

F_err mfs_f_tell(F *file, uint64_t *buffer_position) {
	FAIL_ON(!file);
	FAIL_ON(!file->__open__);
	FAIL_ON(file->__type__ != FILE);
	FAIL_ON(!buffer_position);

	*buffer_position = file->__position__;
	return NO_ERROR;
}

bool mfs_f_eof(F *file) {
	FAIL_ON(!file);
	FAIL_ON(!file->__open__);
	FAIL_ON(file->__type__ != FILE);

	d_f *fdata = (d_f *)file->__data__;

	return file->__position__ == fdata->size;
}

F_err mfs_f_size(F *file, uint64_t *file_size) {
	FAIL_ON(!file);
	FAIL_ON(!file->__open__);
	FAIL_ON(file->__type__ != FILE);
	FAIL_ON(!file_size);

	d_f *fdata = (d_f *)file->__data__;
	*file_size = fdata->size;
	return NO_ERROR;
}


F_err mfs_d_open(F *dir, char *name, uint16_t mode) {
	if (!dir) {
		return FILE_NOT_FOUND;
	}
	map_t _dir = root;
	d_f *dir_or_file = &__root__;
	int entries = strseg_count(name, '/');

	for(int i=0; i<entries; i++) {
		char *entry = strseg(name, '/', i);

		if (hashmap_get(_dir, entry, (void **)&dir_or_file)) {
			kfree(entry);
			goto err;
		}

		if (dir_or_file->type == MAP_FILE) {
			kfree(entry);
			goto err;
		}

		_dir = dir_or_file->dir;
		kfree(entry);
	}

	if (dir_or_file->type != MAP_DIR) {
		goto err;
	}


	spin_lock(&dir_or_file->reflock);
	ref_count_inc(&dir_or_file->refcount);
	spin_unlock(&dir_or_file->reflock);

	dir->__position__ = 0;
	dir->__open__ = true;
	dir->__data__ = dir_or_file;
	dir->__type__ = DIRECTORY;
	dir->fs = filesystem;
	return NO_ERROR;

err:
	return FILE_NOT_FOUND;
}

F_err mfs_d_close(F *dir) {
	dir->fs = NULL;
	dir->__open__ = false;
	dir->__position__ = 0;

	d_f *d_data = (d_f *)dir->__data__;
	dir->__data__ = NULL;

	if (d_data->other != NULL) {
		hashmap_iterator_done(d_data->other);
		d_data->other = NULL;
	}

	spin_lock(&d_data->reflock);
	ref_count_dec(&d_data->refcount);
	spin_unlock(&d_data->reflock);

	return NO_ERROR;
}

F_err mfs_d_next(F *dir, char **name) {
	FAIL_ON(!dir);
	FAIL_ON(!dir->__open__);
	FAIL_ON(dir->__type__ != DIRECTORY);
	d_f *d_data = (d_f *)dir->__data__;

	if (d_data->other == NULL) {
		d_data->other = map_iterator(d_data->dir);
		if (!d_data->other) {
			return FILESYSTEM_ERROR;
		}
	}

	if (hashmap_iterator_has_next(d_data->other)) {
		*name = hashmap_iterator_next(d_data->other)->key;
	} else {
		*name = NULL;
	}
	return NO_ERROR;
}

F_err mfs_d_rewind(F *dir) {
	FAIL_ON(!dir);
	FAIL_ON(!dir->__open__);
	FAIL_ON(dir->__type__ != DIRECTORY);

	d_f *d_data = (d_f *)dir->__data__;

	if (d_data->other != NULL) {
		hashmap_iterator_done(d_data->other);
	}

	d_data->other = map_iterator(d_data->dir);
}

F_err mfs_d_mkdir(F *dir, char *name) {
	FAIL_ON(!dir);
	FAIL_ON(!dir->__open__);
	FAIL_ON(dir->__type__ != DIRECTORY);
	FAIL_ON(!name);

	d_f *d_data = (d_f *)dir->__data__;

	map_t newdirmap = hashmap_new();
	if (!newdirmap) {
		return FILESYSTEM_ERROR;
	}

	d_f *newdir = kmalloc(sizeof(d_f));
	if (!newdir) {
		return FILESYSTEM_ERROR;
	}

	newdir->type = MAP_DIR;
	newdir->size = 0;
	newdir->other = NULL;
	newdir->dir = newdirmap;

	hashmap_put(d_data->dir, name, newdir);
	return NO_ERROR;
}

F_err mfs_d_delete(F *dir, char *name) {
	FAIL_ON(!dir);
	FAIL_ON(!dir->__open__);
	FAIL_ON(dir->__type__ != DIRECTORY);
	FAIL_ON(!name);

	d_f *d_data = (d_f *)dir->__data__;
	FAIL_ON(!d_data);

	map_t *map = d_data->dir;
	FAIL_ON(!map);

	d_f *dir_or_file = NULL;
	if (hashmap_get(map, name, (void **)&dir_or_file)) {
		return FILE_NOT_FOUND;
	}

	spin_lock(&dir_or_file->reflock);
	if(dir_or_file->refcount) {
		spin_unlock(&dir_or_file->reflock);
		return FILE_IN_USE;
	}

	if (hashmap_remove(map, name)) {
		spin_unlock(&dir_or_file->reflock);
		return FILESYSTEM_ERROR;
	}

	if (dir_or_file->type == MAP_DIR) {
		if (hashmap_size(dir_or_file->dir)) {
			spin_unlock(&dir_or_file->reflock);
			return DIRECTORY_NOT_EMPTY;
		}
		if (dir_or_file->other) {
			kfree(dir_or_file->other);
		}
		hashmap_free(dir_or_file->dir);
	} else {
		kfree(dir_or_file->data);
	}

	kfree(dir_or_file);
	spin_unlock(&dir_or_file->reflock);
	return NO_ERROR;
}