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

	F_err (*d_open)(F *dir, char *name, uint16_t mode);
	F_err (*d_close)(F *dir);
	F_err (*d_next)(F *dir, char **name);
	F_err (*d_rewind)(F *dir);
	F_err (*d_mkdir)(F *dir, char *name);

	F_err (*i_stat)(S *stat, char *path);
	F_err (*i_unlink)(char *path);
	F_err (*i_rename)(char *old_path, char *new_path);
} filesystem_t;

*/

F_err kfs_nop() {
	return NOT_IMPLEMENTED_ERROR;
}

filesystem_t *kernel_fs_init() {
	filesystem_t *result = mfs_new_fs();

	result->node_type = mfs_node_type;

	result->f_open = mfs_f_open;
	result->f_close = mfs_f_close;
	result->f_read = mfs_f_read;
	result->f_write = mfs_f_write;

	result->f_lseek = (void *)mfs_f_lseek;
	result->f_tell = (void *)mfs_f_tell;
	result->f_eof = (void *)mfs_f_eof;
	result->f_size = (void *)mfs_f_size;

	result->d_open = (void *)mfs_d_open;
	result->d_close = (void *)mfs_d_close;
	result->d_next = (void *)mfs_d_next;
	result->d_rewind = (void *)mfs_d_rewind;
	result->d_mkdir = (void *)mfs_d_mkdir;


	result->f_truncate = (void *)kfs_nop;
	result->f_sync = (void *)kfs_nop;

	return result;
}