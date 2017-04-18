#ifndef sfs_h
#define sfs_h

#include <ktype.h>
#include <ata.h>
#include <filesystem.h>


#define CHUNK_SIZE 512
#define VERSION 0x1

typedef struct {
	uint8_t __raw[512];
}__attribute__((packed)) chunk_t;

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
fs_t get_fs_ata(struct ata_device *);

void mkfs_sfs(fs_t *fs);
bool detectfs(fs_t *fs);


#endif