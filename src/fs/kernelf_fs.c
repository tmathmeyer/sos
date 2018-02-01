#include <fs/kernel_fs.h>
#include <fs/fs.h>
#include <fs/mfs/map_fs.h>


/*
typedef
struct filesystem_s {
	F_err (*f_open)(F *file, char *name, uint16_t mode);
	F_err (*f_close)(F *file);
	F_err (*f_read)(F *file, void *dest, uint64_t to_read, uint64_t *actually_read);
	F_err (*f_write)(F *file, void *src, uint64_t to_write, uint64_t *actually_wrote);
	F_err (*f_lseek)(F *file, uint64_t seek_to);
	F_err (*f_truncate)(F *file);
	F_err (*f_sync)(F *file);
	F_err (*f_tell)(F *file, uint64_t *buffer_position);
	F_err (*f_eof)(F *file);
	F_err (*f_size)(F *file, uint64_t *file_size);

	F_err (*d_open)(D *dir, char *name, uint16_t mode);
	F_err (*d_close)(D *dir);
	F_err (*d_next)(D *dir, char **name);
	F_err (*d_rewind)(D *dir);
	F_err (*d_mkdir)(char *name);

	F_err (*i_stat)(S *stat, char *path);
	F_err (*i_unlink)(char *path);
	F_err (*i_rename)(char *old_path, char *new_path);
} filesystem_t;

*/

filesystem_t *kernel_fs_init() {
	filesystem_t *result = mfs_new_fs();

	result->f_open = mfs_open;
	result->f_close = mfs_close;
	result->f_read = mfs_read;
	result->f_write = mfs_write;

	return result;
}