#ifndef fs_h
#define fs_h

#include <ktype.h>

typedef uint32_t FLAGS;
typedef uint32_t fs_type;

#define F_OPT_CREATE 0x01 << 0
#define F_OPT_TYPE_DIR 0x01 << 1
#define F_OPT_TYPE_FILE 0x01 << 2
#define F_OPT_TYPE_MOUNT 0x01 << 3

#define FTYPE_FILE  0x01
#define FTYPE_DIR   0x02
#define FTYPE_MOUNT 0x03


typedef enum FS_ERROR {
	NO_ERROR = 0,
	FILE_NOT_FOUND = 1,
	DISK_READ_ERROR = 2,
	DISK_WRITE_ERROR = 3,
	NOT_A_DIRECTORY = 4
} FS_ERROR;

typedef
struct fs_disk_s {
	/* a disk has 32 bytes to store all of its metadata */
	uint8_t _data[32];

	/* read and write operations on a disk */
	uint64_t (*write_offset)(struct fs_disk_s *, uint64_t offset, void *src, uint64_t len);
	uint64_t (*read_offset)(struct fs_disk_s *, uint64_t offset, void *dest, uint64_t len);
	/* get size of the disk IN BYTES */
	uint64_t (*get_size)(struct fs_disk_s *);
}__attribute__((packed)) fs_disk_t;

typedef
struct mountfile_s {
	struct fs_file_s *fs;
}__attribute__((packed)) mountfile_t;

typedef
struct dirent_s {
	uint64_t _table_block;
	uint64_t _table_entry;
	uint64_t name_length;
	char name[0];
}__attribute__((packed)) dirent_t;

typedef
struct directory_s {
	uint64_t entry_count;
	dirent_t entries[0];
}__attribute__((packed)) directory_t;

typedef
struct fs_file_s {
	/* the backend */
	struct fs_s *_fs;
	uint64_t _position;
	uint8_t _data[64];

	void (*close)(struct fs_file_s *);

	void (*rewind)(struct fs_file_s *);
	void (*seekend)(struct fs_file_s *);

	uint64_t (*read)(struct fs_file_s *, void *dest, uint64_t len);

	uint64_t (*write)(struct fs_file_s *, void *src, uint64_t len);
}__attribute__((packed)) fs_file_t;

typedef
struct fs_metadata_s {
	uint8_t rwx : 3;
	uint8_t _reserved : 5;
	fs_type type;
	uint64_t UTC_touched;
	uint64_t UTC_created;
	uint64_t size_bytes;
}__attribute__((packed)) fs_metadata_t;

typedef
struct fs_s {
	/* for checking filesystem types */
	char *(*identify)(void);

	/* open a file */
	FS_ERROR (*open)(struct fs_s *, char *path, fs_file_t *entry, FLAGS flags);

	/* get statistics about a file */
	FS_ERROR (*stat)(struct fs_s *, char *path, fs_metadata_t *metadata);

	/* delete a file. If the file is a list, delete recursively */
	FS_ERROR (*remove)(struct fs_s *, char *path);

	/* make an instance of this filesystem on the store */
	FS_ERROR (*mkfs)(struct fs_s *, fs_disk_t *store);

	/* the backend */
	fs_disk_t *_disk;

	/* 3 pointers worth of extra information */
	void *_metadata[2];
}__attribute__((packed)) fs_t;

#endif
