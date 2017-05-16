#ifndef reefs_h
#define reefs_h

#include "fs.h"

typedef struct {uint8_t x[512];} chunk_t;
#define MAGIC 0x7265656673

typedef union {
	struct {
		uint64_t magic_number;      // ID the filesystem: "0x72 65 65 66 73"
	    uint64_t version;           // version of filesystem
	    uint64_t table_chunk_count; // the number of chunks in top table
	    uint64_t file_chunk_count;  // the number of chunks used for files
	    uint64_t total_chunk_count; // the total number of physical chunks on the disk
	};
	chunk_t __align;
}__attribute__((packed)) header_chunk_t;

typedef struct {
	uint16_t occupied_bytes : 9;
	uint64_t data_checksum : 56;
	uint64_t next_chunk : 56;
	uint8_t zeroes : 7;
	uint8_t data[496];
}__attribute__((packed)) file_chunk_t;

typedef struct { // 16 bytes
	uint8_t rwx : 3;
	uint8_t type : 5;
	uint64_t size : 64;
	uint64_t first_chunk : 56;
}__attribute__((packed)) file_header_t;

typedef struct {
	struct { // 16 bytes
		uint32_t in_use;
		uint32_t chunk_present_mask;
		uint64_t checksum;
	};
	file_header_t headers[31]; // 31 * 16 + 16 = 512
}__attribute__((packed)) table_chunk_t;

fs_t empty_fs(void);

#endif