#ifndef filesystem_h
#define filesystem_h

#include <libk.h>
#include <ktype.h>

typedef int FS_ERROR;
typedef uint8_t node_type;

#define NO_ERROR 0
#define FILE_NOT_FOUND 1
#define FILE_NOT_MODIFIABLE 2
#define NO_ACTION 3
#define NOT_A_DIRECTORY 4
#define NOT_A_FILE 5
#define NOT_ENOUGH_SPACE 6
#define ERROR_CRIT 99

#define TYPE_NONE 0
#define TYPE_FILE 1
#define TYPE_DIR 2
#define TYPE_MOUNT 3


#define inode_name_max 28
typedef struct {
	uint8_t name[inode_name_max];
	uint64_t table_entry;
	uint32_t table_slot;
}__attribute__((packed)) inode_entry_t;

typedef struct inode_s {
	struct _fs_t *fs;
	struct {
		uint64_t entry_count;
		inode_entry_t entries[0];
	} data;
}__attribute__((packed)) inode_t;

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

	/* delete a file */
	FS_ERROR (*try_delete_at)(struct _fs_t*, uint64_t, uint32_t);

	/* get the type of node at the the entry */
	FS_ERROR (*get_type)(struct _fs_t *, uint64_t, uint32_t, node_type *);

	/* get the size of the file */
	FS_ERROR (*get_size)(struct _fs_t *, uint64_t, uint32_t, uint64_t *);

	/* read a directory from an entry */
	FS_ERROR (*read_dir_at)(struct _fs_t *, uint64_t, uint32_t, inode_t *);

	/* write a directory from an entry */
	FS_ERROR (*write_dir_at)(struct _fs_t *, uint64_t, uint32_t, inode_t *);
	FS_ERROR (*write_new_dir)(struct _fs_t *, uint64_t *, uint32_t *, inode_t *);

	/* read file contents */
	FS_ERROR (*read_file_at)(struct _fs_t *, uint64_t, uint32_t, uint64_t, uint64_t *, void *);

	/* write file contents */
	FS_ERROR (*write_file_at)(struct _fs_t *, uint64_t, uint32_t, uint64_t, void *);
	FS_ERROR (*write_new_file)(struct _fs_t *, uint64_t*, uint32_t*, uint64_t, void *);

}__attribute__((packed)) fs_t;


/* mount a filesystem */
void mount_fs(inode_t *entry, char *location);

#endif