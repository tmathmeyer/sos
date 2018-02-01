#ifndef mapfs_h
#define mapfs_h

#include <fs/fs.h>
#include <std/int.h>
#include <std/map.h>

typedef
enum {
	FILE,
	DIR
} TYPE;

typedef
struct {
	union {
		map_t dir;
		char *data;
	};
	uint64_t size;
	TYPE type;
} d_f;


F_err mfs_open(F *file, char *name, uint16_t mode);
F_err mfs_close(F *file);
F_err mfs_read(F *file, void *dest, uint64_t to_read, uint64_t *actually_read);
F_err mfs_write(F *file, void *src, uint64_t to_write, uint64_t *actually_wrote);

filesystem_t *mfs_new_fs();


#endif