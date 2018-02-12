#ifndef mapfs_h
#define mapfs_h

#include <fs/fs.h>
#include <std/int.h>
#include <std/map.h>
#include <arch/lock.h>

typedef
enum {
	MAP_FILE,
	MAP_DIR,
	MAP_BLOCK,
} TYPE;

typedef
struct d_f_s{
	union {
		map_t dir;
		char *data;
	};
	void *other;
	uint64_t size;
	TYPE type;
	lock_t reflock;
	ref_t refcount;
} d_f;

F_type mfs_node_type(char *name);

F_err mfs_f_open(F *file, char *name, uint16_t mode);
F_err mfs_f_close(F *file);
F_err mfs_f_read(F *file, void *dest, uint64_t to_read, uint64_t *actually_read);
F_err mfs_f_write(F *file, void *src, uint64_t to_write, uint64_t *actually_wrote);
F_err mfs_f_lseek(F *file, uint64_t seek_to);
F_err mfs_f_tell(F *file, uint64_t *buffer_position);
bool mfs_f_eof(F *file);
F_err mfs_f_size(F *file, uint64_t *file_size);

F_err mfs_d_open(F *dir, char *name, uint16_t mode);
F_err mfs_d_close(F *dir);
F_err mfs_d_next(F *dir, char **name);
F_err mfs_d_rewind(F *dir);
F_err mfs_d_mkdir(F *dir, char *name);
F_err mfs_d_delete(F *dir, char *name);


filesystem_t *mfs_new_fs();


#endif