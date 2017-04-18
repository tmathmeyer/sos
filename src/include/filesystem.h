#ifndef filesystem_h
#define filesystem_h

#include <libk.h>
#include <ktype.h>

typedef int FS_ERROR;

#define NO_ERROR 0
#define FILE_NOT_FOUND 1
#define FILE_NOT_MODIFIABLE 2
#define NO_ACTION 3

/* a filesystem */
typedef struct _fs_t{
	/* protected. do not mess with unless you are a filesystem */
	void *_underlying_data;
	void *_UD2;
	void (*_chunk_read)(struct _fs_t *fs, uint64_t chunk_no, void *data);
	void (*_chunk_write)(struct _fs_t *fs, uint64_t chunk_no, void *data);

	/* get the size of the disk */
	uint64_t (*get_fs_size_bytes)(struct _fs_t *fs);

	/* for matching the fs */
	bool (*identity)(char *name);

	/* do we have a file by that name */
	bool (*file_exists)(struct _fs_t *fs, char *name);

	/* write data to file */
	FS_ERROR (*file_write)(struct _fs_t *fs, char *name, void *data, uint64_t len);
	
	/* delete a file */
	FS_ERROR (*file_delete)(struct _fs_t *fs, char *name);
	
	/* read from a file */
	FS_ERROR (*file_read)(struct _fs_t *fs, char *name, void *data, uint64_t max_len, uint64_t *read);
}__attribute__((packed)) fs_t;

/* mount a filesystem */
void mount_fs(fs_t *entry, char *location);

/* unmount a filesystem */
void unmount_fs(char *location);

/* start the vfs */
void fs_init(fs_t *rootfs);

#endif