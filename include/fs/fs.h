#ifndef fs_h
#define fs_h

#include <std/int.h>


typedef
enum {
	NO_ERROR = 0,
	FILE_NOT_FOUND = 1,
	ARGUMENT_ERROR = 2,
} F_err;

typedef
struct {
	uint8_t __open__;
	uint64_t __position__;
	void *__data__;
	struct filesystem_s *fs;
} F;

typedef
struct _directory {
	uint64_t __position__;
} D;

typedef
struct _stat {
	uint64_t __position__;
} S;

typedef
struct filesystem_s {
	/* File operations */
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

	/* Directory operations */
	F_err (*d_open)(D *dir, char *name, uint16_t mode);
	F_err (*d_close)(D *dir);
	F_err (*d_next)(D *dir, char **name);
	F_err (*d_rewind)(D *dir);
	F_err (*d_mkdir)(char *name);

	/* Status operations */
	F_err (*i_stat)(S *stat, char *path);
	F_err (*i_unlink)(char *path);
	F_err (*i_rename)(char *old_path, char *new_path);

	void *__internal__;
} filesystem_t;


#endif