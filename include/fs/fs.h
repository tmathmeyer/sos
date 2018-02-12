#ifndef fs_h
#define fs_h

#include <std/int.h>


#define CREATE_ON_OPEN 0x01
#define CREATE_BLOCK_DEVICE 0x02

typedef
enum {
	NO_ERROR = 0,
	FILE_NOT_FOUND = 1,
	ARGUMENT_ERROR = 2,
	NOT_IMPLEMENTED_ERROR = 3,
	FILESYSTEM_ERROR = 4,
	FILE_IN_USE = 5,
	DIRECTORY_NOT_EMPTY = 6,
} F_err;

typedef
enum {
	FILE = 0,
	DIRECTORY = 1,
	SYMLINK = 2,
	BLOCK_DEVICE = 3,
	INVALID = 4,
} F_type;

typedef
struct {
	uint8_t __open__;
	uint64_t __position__;
	void *__data__;
	struct filesystem_s *fs;
	F_type __type__;
	char *opened_as;
} F;

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
	bool  (*f_eof)(F *file);
	F_err (*f_size)(F *file, uint64_t *file_size);

	/* Directory operations */
	F_err (*d_open)(F *dir, char *name, uint16_t mode);
	F_err (*d_close)(F *dir);
	F_err (*d_next)(F *dir, char **name);
	F_err (*d_rewind)(F *dir);
	F_err (*d_mkdir)(F *dir, char *name);
	F_err (*d_delete)(F *dir, char *name);

	/* Status operations */
	F_err (*i_stat)(S *stat, char *path);
	F_err (*i_unlink)(char *path);
	F_err (*i_rename)(char *old_path, char *new_path);

	F_type (*node_type)(char *path);

	void *__internal__;
} filesystem_t;

typedef
struct block_device_s {
	uint64_t (*read)(struct block_device_s *, uint64_t offset, uint64_t length, uint8_t *dest);
	uint64_t (*write)(struct block_device_s *, uint64_t offset, uint64_t length, uint8_t *src);
	void *__private__;
} block_device_t;


#endif