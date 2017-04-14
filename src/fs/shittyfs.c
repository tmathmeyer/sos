#ifdef STDLIB
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/shittyfs.h"
#else
#include "ktype.h"
#include "libk.h"
#include "kio.h"
#include "shittyfs.h"
#endif

union {
	uint64_t number;
	uint8_t __txt[8];
} magic = {
	.__txt = "SHiTTYfs"
};

bool detectfs(fs_t *fs) {
	system_desc_chunk_t system_chunk;
	fs->chunk_read(fs, 0, &system_chunk);

	if (system_chunk.magic_number != magic.number) {
		return false;
	}

	if (system_chunk.version != VERSION) {
		return false;
	}

	return true;
}

void mkfs(fs_t *fs) {
	system_desc_chunk_t system_chunk = {
		.__padding = {0}
	};

	system_chunk.magic_number = magic.number;
	system_chunk.version = VERSION;
	system_chunk.table_chunk_count = 0;
	system_chunk.file_chunk_count = 0;
	system_chunk.total_chunk_count = fs->total_chunk_count;

	fs->chunk_write(fs, 0, &system_chunk);
	
}

uint64_t checksum(uint8_t *data, uint64_t bytes) {
	uint64_t result = 0;
	for(uint64_t i=0; i<bytes; i++) {
		result ^= data[i];
		uint64_t msb = result >> 63;
		result = (result<<1 | msb);
	}
}

uint64_t sfs_get_next_file_entry_chunk_no(fs_t *fs, uint8_t *open_spot) {
	system_desc_chunk_t system_chunk;
	fs->chunk_read(fs, 0, &system_chunk);

	if (system_chunk.magic_number != magic.number) {
		//PANIC!
	}

	table_chunk_t table_chunk;
	uint64_t chunk_no = 1 + system_chunk.table_chunk_count;

	if (system_chunk.table_chunk_count == 0) {
create_new_table:
		memset(&table_chunk, 0, CHUNK_SIZE);
		table_chunk.__used = 1;
		fs->chunk_write(fs, chunk_no, &table_chunk);
		*open_spot = 0;
		return chunk_no;
	}

	fs->chunk_read(fs, chunk_no, &table_chunk);
	if (table_chunk.__chunk_present_mask == 0xF) {
		chunk_no++;
		goto create_new_table;
	}

	if (!(0x1 & table_chunk.__chunk_present_mask)) {
		*open_spot = 0;
	} else if (!(0x1 & table_chunk.__chunk_present_mask)) {
		*open_spot = 1;
	} else if (!(0x1 & table_chunk.__chunk_present_mask)) {
		*open_spot = 2;
	} else if (!(0x1 & table_chunk.__chunk_present_mask)) {
		*open_spot = 3;
	}

	return chunk_no;
}

int is_valid_occupied_file_chunk(file_chunk_t chunk) {
	if (chunk.occupied_bytes == 0) {
		return 0;
	}
	if (chunk.zeroes) {
		return 0;
	}
	if (checksum(chunk.data, 496) != chunk.data_checksum) {
		return 0;
	}
	return 1;
}

void sfs_file_write(fs_t *fs, char *name, void *data, uint64_t len) {
	if (len == 0) {
		return; //TODO: return error code
	}

	system_desc_chunk_t system_chunk;
	fs->chunk_read(fs, 0, &system_chunk);

	uint8_t chunk_free_spot = 0;
	uint64_t table_chunk_no = sfs_get_next_file_entry_chunk_no(fs, &chunk_free_spot);
	system_chunk.table_chunk_count = table_chunk_no;
	uint64_t backup_len = len;

	uint64_t file_chunks_needed = (len + CHUNK_SIZE - 17) / (CHUNK_SIZE - 16);
	
	uint64_t free_chunks = system_chunk.total_chunk_count - 
			(system_chunk.table_chunk_count + system_chunk.file_chunk_count);

	if (free_chunks < file_chunks_needed) {
		return; //TODO: return error code
	}

	system_chunk.file_chunk_count += file_chunks_needed;

	uint64_t first_chunk_no = 0;

	file_chunk_t prev;
	uint64_t prev_chunk_no = 0;
	for(int i=1; i<=file_chunks_needed; i++) {
		file_chunk_t test;
		uint64_t cid = system_chunk.total_chunk_count-i;
		fs->chunk_read(fs, cid, &test);
		if (!is_valid_occupied_file_chunk(test)) {
			if (!first_chunk_no) {
				first_chunk_no = cid;
			}
			test.occupied_bytes = min(len, (CHUNK_SIZE-16));
			test.zeroes = 0;
			memcpy(test.data, data, test.occupied_bytes);
			test.data_checksum = checksum(test.data, CHUNK_SIZE-16);
			if (prev_chunk_no) {
				prev.next_chunk = cid;
				fs->chunk_write(fs, prev_chunk_no, &prev);
			}
			prev_chunk_no = cid;
			prev = test;
			fs->chunk_write(fs, cid, &test);
			len -= (CHUNK_SIZE - 16);
			data += (CHUNK_SIZE - 16);
		}
	}

	table_chunk_t table_chunk;
	fs->chunk_read(fs, table_chunk_no, &table_chunk);
	memcpy(table_chunk.headers[chunk_free_spot].name, name, strlen(name));
	table_chunk.headers[chunk_free_spot].read = 1;
	table_chunk.headers[chunk_free_spot].write = 1;
	table_chunk.headers[chunk_free_spot].execute = 1;
	table_chunk.headers[chunk_free_spot].type = 0x01; //FILE
	table_chunk.headers[chunk_free_spot].size = backup_len;
	table_chunk.headers[chunk_free_spot].first_chunk = first_chunk_no;
	table_chunk.__chunk_present_mask |= 0x1<<chunk_free_spot;
	fs->chunk_write(fs, 0, &system_chunk);
	fs->chunk_write(fs, table_chunk_no, &table_chunk);
}

uint64_t sfs_read_file_data(fs_t *fs, file_header_t hdr, void *data, uint64_t max_len) {
	uint64_t readleft = min(hdr.size, max_len);
	uint64_t read_total = readleft;
	uint64_t chunk_no = hdr.first_chunk;
	file_chunk_t filechunk;
	while(readleft && readleft <= read_total) {
		fs->chunk_read(fs, chunk_no, &filechunk);
		memcpy(data, filechunk.data, filechunk.occupied_bytes);
		data += filechunk.occupied_bytes;
		readleft -= filechunk.occupied_bytes;
		chunk_no = filechunk.next_chunk;
	}
	return read_total;
}

uint64_t sfs_file_read(fs_t *fs, char *name, void *data, uint64_t max_len) {
	system_desc_chunk_t system_chunk;
	fs->chunk_read(fs, 0, &system_chunk);

	if (system_chunk.table_chunk_count == 0) {
		return 0;
	}

	for(uint64_t i=1; i<=system_chunk.table_chunk_count; i++) {
		table_chunk_t table_chunk;
		fs->chunk_read(fs, i, &table_chunk);
		uint8_t flag = 0x1;
		uint8_t shift = 0;

		while(shift < 4) {
			if (table_chunk.__chunk_present_mask & (flag<<shift)) {
				char *nametest = table_chunk.headers[shift].name;
				if (!strncmp(nametest, name, strlen(nametest))) {
					return sfs_read_file_data(fs, table_chunk.headers[shift], data, max_len);
				}
			}
			shift++;
		}
	}
	return 0;
}

void memory_chunk_read(fs_t *fs, uint64_t chunk_no, void *data) {
	chunk_t *read_from = ((chunk_t *)(fs->underlying_data))+chunk_no;
	memcpy(data, read_from, CHUNK_SIZE);
}

void memory_chunk_write(fs_t *fs, uint64_t chunk_no, void *data) {
	chunk_t *write_to = ((chunk_t *)(fs->underlying_data))+chunk_no;
	memcpy(write_to, data, CHUNK_SIZE);
}

fs_t get_fs_memory(uint8_t *buffer, uint64_t bytes) {
	fs_t result;
	result.magic_number = magic.number;
	result.underlying_data = buffer;
	result.chunk_read = memory_chunk_read;
	result.chunk_write = memory_chunk_write;
	result.file_read = sfs_file_read;
	result.file_write = sfs_file_write;
	result.total_chunk_count = bytes / CHUNK_SIZE;
	return result;
}

uint64_t file_read(fs_t *fs, char *name, void *data, uint64_t max_len) {
	return fs->file_read(fs, name, data, max_len);
}

void file_write(fs_t *fs, char *name, void *data, uint64_t len) {
	fs->file_write(fs, name, data, len);
}







#ifndef STDLIB

void ata_chunk_read(fs_t *fs, uint64_t chunk_no, void *data) {
	ata_device_read_sector(fs->underlying_data, chunk_no, data);
}

void ata_chunk_write(fs_t *fs, uint64_t chunk_no, void *data) {
	ata_device_write_sector_retry(fs->underlying_data, chunk_no, data);
}

fs_t get_fs_ata(struct ata_device *dev) {
	fs_t result;
	result.magic_number = magic.number;
	result.underlying_data = dev;
	result.chunk_read = ata_chunk_read;
	result.chunk_write = ata_chunk_write;
	result.file_read = sfs_file_read;
	result.file_write = sfs_file_write;
	result.total_chunk_count = ata_max_offset(dev) / ATA_SECTOR_SIZE;
	return result;
}

#else

void print_filesystem_debug(fs_t *fs) {
	system_desc_chunk_t s;
	fs->chunk_read(fs, 0, &s);

	printf("magic_number      = %i\n", s.magic_number);
	printf("version           = %i\n", s.version);
	printf("table_chunk_count = %i\n", s.table_chunk_count);
	printf("file_chunk_count  = %i\n", s.file_chunk_count);
	printf("total_chunk_count = %i\n", s.total_chunk_count);

	for(int i=1; i<=s.table_chunk_count; i++) {
		table_chunk_t table_chunk;
		fs->chunk_read(fs, i, &table_chunk);
		uint8_t flag = 0x1;
		uint8_t shift = 0;
		printf("table #%i mask is %i\n", i, table_chunk.__chunk_present_mask);
		while(shift < 4) {
			if (table_chunk.__chunk_present_mask & (flag<<shift)) {
				char *nametest = table_chunk.headers[shift].name;
				uint64_t firsttest = table_chunk.headers[shift].first_chunk;
				uint64_t sizetest = table_chunk.headers[shift].size;
				printf("file entry (%i, %i):\n", i, shift);
				printf("  name: %s\n", nametest);
				printf("  fchk: %i\n", firsttest);
				printf("  size: %i\n", sizetest);
			} else {
				printf("file entry (%i, %i) is empty\n", i, shift);
			}
			shift++;
		}
	}
}

int main() {
	uint8_t *mem = malloc(CHUNK_SIZE * 2000);

	fs_t filesystem = get_fs_memory(mem, CHUNK_SIZE * 2000);
	char *LOLFILE = "Lorem Ipsem is one hell of a drug\n"
					"  by Ted Meyer\n"
					"\n"
					"Lorem Ipsum makes you high as a kite lol\n"
					"Lorem Ipsum makes you win your fights lol\n"
					"Lorem Ipsum is this stupif fucking poem over\n"
					"Lorem Ipsum what the fuck did I do wrong in\n"
					"That releationship\n"
					"\n"
					"The plan to get her back:\n"
					"   Workout - pretty obvious, goal is 100 of each\n"
					"             curlups, pushups, squats, and lifts a day\n"
					"   Habits - stop biting nails\n"
					"   Wine - try two ~60$ bottles, pick best, mail it to her\n"
					"   Card - send nice handwritten card with bottle of wine\n"
					"   Try not to fall apart and become suicidal when she says no\n";


	file_write(&filesystem, "poem.notes", LOLFILE, strlen(LOLFILE));
	char data[800] = {0};

	uint64_t read_back = file_read(&filesystem, "poem.notes", data, 800);
	printf("When reading back in we got %i bytes\n", read_back);
	puts(data);
}
#endif
