#ifndef shittyfs_h
#define shittyfs_h


#ifdef STDLIB
#include <stdint.h>
#else
#include "ktype.h"
#include "ata.h"
#endif


#define CHUNK_SIZE 512
#define VERSION 0x1

typedef struct {
	uint8_t __raw[512];
}__attribute__((packed)) chunk_t;

typedef struct _fs_t{
	uint64_t magic_number;
	uint64_t total_chunk_count;
	void *underlying_data;
	void (*chunk_read)(struct _fs_t *fs, uint64_t chunk_no, void *data);
	void (*chunk_write)(struct _fs_t *fs, uint64_t chunk_no, void *data);
	void (*file_write)(struct _fs_t *fs, char *name, void *data, uint64_t len);
	uint64_t (*file_read)(struct _fs_t *fs, char *name, void *data, uint64_t max_len);
}__attribute__((packed)) fs_t;

typedef union {
	struct {
		uint64_t magic_number;      // ID the filesystem: "shittyfs"
	    uint64_t version;           // version of filesystem
	    uint64_t table_chunk_count; // the number of chunks in top table
	    uint64_t file_chunk_count;  // the number of chunks used for files
	    uint64_t total_chunk_count; // the total number of physical chunks on the disk
	};
	uint8_t __padding[512];
}__attribute__((packed)) system_desc_chunk_t;

typedef struct {
	uint8_t name[110];
	uint8_t read : 1;
	uint8_t write : 1;
	uint8_t execute : 1;
	uint8_t type : 5;
	uint64_t size : 64;
	uint64_t first_chunk : 56;
}__attribute__((packed)) file_header_t;

typedef struct {
	struct {
		uint8_t __used : 1;
		uint8_t __chunk_present_mask : 4;
		uint8_t __zeroes : 1;
		uint64_t __checksum : 56;
	};
	file_header_t headers[4];
}__attribute__((packed)) table_chunk_t;

typedef struct {
	uint16_t occupied_bytes : 9;
	uint64_t data_checksum : 56;
	uint64_t next_chunk : 56;
	uint8_t zeroes : 7;
	uint8_t data[496];
}__attribute__((packed)) file_chunk_t;


fs_t get_fs_memory(uint8_t *, uint64_t);
void mkfs(fs_t *fs);
bool detectfs(fs_t *fs);
uint64_t file_read(fs_t *, char *, void *, uint64_t);

void file_write(fs_t *, char *, void *, uint64_t);

#ifndef STDLIB
fs_t get_fs_ata(struct ata_device *);
#endif

#endif