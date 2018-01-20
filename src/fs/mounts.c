/*
/* File operations 
F_err f_open(F *file, char *name, uint16_t mode);
F_err f_close(F *file);
F_err f_read(F *file, void *dest, uint64_t to_read, uint64_t *actually_read);
F_err f_write(F *file, void *src, uint64_t to_write, uint64_t *actually_wrote);
F_err f_lseek(F *file, uint64_t seek_to);
F_err f_truncate(F *file);
F_err f_sync(F *file);
F_err f_tell(F *file, uint64_t *buffer_position);
F_err f_eof(F *file);
F_err f_size(F *file, uint64_t *file_size);

/* Directory operations 
F_err d_open(D *dir, char *name, uint16_t mode);
F_err d_close(D *dir);
F_err d_next(D *dir, char **name);
F_err d_rewind(D *dir);
F_err d_mkdir(char *name);

F_err i_stat(S *stat, char *path);
F_err i_unlink(char *path);
F_err i_rename(char *old_path, char *new_path);
*/

#include <fs_api.h>
#include <ctree.h>
#include <ktype.h>
#include <alloc.h>
#include <path.h>

#define fail_on(e) do {\
	F_err __ERROR_GEN=(e); \
	if(__ERROR_GEN) return __ERROR_GEN; \
} while(0)


ctree_t *ROOT = NULL;


filesystem_t *create_root_filesystem(void) {
	filesystem_t *result = kmalloc(sizeof(filesystem_t *));
	result->f_open = root_f_open;
	result->f_close = root_f_close;
	result->f_read = root_f_read;

	ROOT = new_ctree(create_memory_filesystem());

	return result;
}

F_err _highest_mount(path_t *path, filesystem_t *fs) {
	char *segment;
	ctree_t *tree = ROOT;
	path_for_each(path, segment) {
		ctree_t *next = ctree_get_element(tree, segment, ctree_string_compare);
		if (next) {
			tree = next;
		} else {
			path->section--;
			*fs = *((filesystem_t *)tree->data);
			return NO_ERROR;
		}
	}
	return FILE_NOT_FOUND;
}

F_err root_f_open(F *file, char *name, uint16_t mode) {
	path_t *path = _allocate_path(name);
	filesystem_t fs;

	fail_on(_highest_mount(path, &fs));
	fail_on(fs.f_open(file, name, mode));
	return NO_ERROR;
}

F_err root_f_close(F *file) {
	fail_on(file->__fs__->f_close(file));
	return NO_ERROR;
}

F_err root_f_read(F *file, void *dest, uint64_t to_read, uint64_t *did_read) {
	fail_on(file->__fs__->f_read(file, dest, to_read, did_read));
	return NO_ERROR;
}

