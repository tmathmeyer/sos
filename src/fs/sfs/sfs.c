#include <ktype.h>
#include <libk.h>
#include <kio.h>
#include <sfs.h>
#include <filesystem.h>

union {
	uint64_t number;
	uint8_t __txt[8];
} magic = {
	.__txt = "SHiTTYfs"
};

bool detectfs(fs_t *fs) {
	system_desc_chunk_t system_chunk;
	fs->_chunk_read(fs, 0, &system_chunk);

	if (system_chunk.magic_number != magic.number) {
		return false;
	}

	if (system_chunk.version != VERSION) {
		return false;
	}

	return true;
}

void mkfs_sfs(fs_t *fs) {
	system_desc_chunk_t system_chunk = {
		.__padding = {0}
	};

	system_chunk.magic_number = magic.number;
	system_chunk.version = VERSION;
	system_chunk.table_chunk_count = 0;
	system_chunk.file_chunk_count = 0;
	system_chunk.total_chunk_count = fs->get_fs_size_bytes(fs) / CHUNK_SIZE;
	fs->_chunk_write(fs, 0, &system_chunk);
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
	fs->_chunk_read(fs, 0, &system_chunk);

	if (system_chunk.magic_number != magic.number) {
		//PANIC!
	}

	table_chunk_t table_chunk;
	uint64_t chunk_no = 1;

	if (system_chunk.table_chunk_count == 0) {
		system_chunk.table_chunk_count = 1;
		fs->_chunk_write(fs, 0, &system_chunk);
create_new_table:
		memset(&table_chunk, 0, CHUNK_SIZE);
		table_chunk.__used = 1;
		fs->_chunk_write(fs, chunk_no, &table_chunk);
		*open_spot = 0;
		return chunk_no;
	}

	fs->_chunk_read(fs, chunk_no, &table_chunk);
	if (table_chunk.__chunk_present_mask == 0xF) {
		chunk_no++;
		goto create_new_table;
	}

	if (!(0x1 & table_chunk.__chunk_present_mask)) {
		*open_spot = 0;
	} else if (!(0x2 & table_chunk.__chunk_present_mask)) {
		*open_spot = 1;
	} else if (!(0x4 & table_chunk.__chunk_present_mask)) {
		*open_spot = 2;
	} else if (!(0x8 & table_chunk.__chunk_present_mask)) {
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

FS_ERROR sfs_file_write(fs_t *fs, char *name, void *data, uint64_t len) {
	if (len == 0) {
		return NO_ACTION;
	}

	fs->file_delete(fs, name);

	system_desc_chunk_t system_chunk;
	fs->_chunk_read(fs, 0, &system_chunk);

	uint8_t chunk_free_spot = 0;
	uint64_t table_chunk_no = sfs_get_next_file_entry_chunk_no(fs, &chunk_free_spot);
	system_chunk.table_chunk_count = table_chunk_no;
	uint64_t backup_len = len;

	uint64_t file_chunks_needed = (len + CHUNK_SIZE - 17) / (CHUNK_SIZE - 16);
	
	uint64_t free_chunks = system_chunk.total_chunk_count - 
			(system_chunk.table_chunk_count + system_chunk.file_chunk_count);

	if (free_chunks < file_chunks_needed) {
		return FILE_NOT_FOUND;
	}

	system_chunk.file_chunk_count += file_chunks_needed;

	uint64_t first_chunk_no = 0;

	file_chunk_t prev;
	uint64_t prev_chunk_no = 0;
	for(int i=1; i<=file_chunks_needed; i++) {
		file_chunk_t test;
		uint64_t cid = system_chunk.total_chunk_count-i;
		fs->_chunk_read(fs, cid, &test);
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
				fs->_chunk_write(fs, prev_chunk_no, &prev);
			}
			prev_chunk_no = cid;
			prev = test;
			fs->_chunk_write(fs, cid, &test);
			len -= (CHUNK_SIZE - 16);
			data += (CHUNK_SIZE - 16);
		}
	}

	table_chunk_t table_chunk;
	fs->_chunk_read(fs, table_chunk_no, &table_chunk);
	memcpy(table_chunk.headers[chunk_free_spot].name, name, strlen(name));
	table_chunk.headers[chunk_free_spot].read = 1;
	table_chunk.headers[chunk_free_spot].write = 1;
	table_chunk.headers[chunk_free_spot].execute = 1;
	table_chunk.headers[chunk_free_spot].type = 0x01; //FILE
	table_chunk.headers[chunk_free_spot].size = backup_len;
	table_chunk.headers[chunk_free_spot].first_chunk = first_chunk_no;
	table_chunk.__chunk_present_mask |= 0x1<<chunk_free_spot;
	fs->_chunk_write(fs, 0, &system_chunk);
	fs->_chunk_write(fs, table_chunk_no, &table_chunk);

	return NO_ERROR;
}

uint64_t sfs_read_file_data(fs_t *fs, file_header_t hdr, void *data, uint64_t max_len) {
	uint64_t readleft = min(hdr.size, max_len);
	uint64_t read_total = readleft;
	uint64_t chunk_no = hdr.first_chunk;
	file_chunk_t filechunk;
	while(readleft && readleft <= read_total) {
		fs->_chunk_read(fs, chunk_no, &filechunk);
		memcpy(data, filechunk.data, filechunk.occupied_bytes);
		data += filechunk.occupied_bytes;
		readleft -= filechunk.occupied_bytes;
		chunk_no = filechunk.next_chunk;
	}
	return read_total;
}

uint64_t get_file_header_by_name(fs_t *fs, char *name, uint8_t *entry) {
	system_desc_chunk_t system_chunk;
	fs->_chunk_read(fs, 0, &system_chunk);

	if (system_chunk.table_chunk_count == 0) {
		return 0;
	}

	if (!entry) {
		return 0;
	}

	for(uint64_t i=1; i<=system_chunk.table_chunk_count; i++) {
		table_chunk_t table_chunk;
		fs->_chunk_read(fs, i, &table_chunk);
		uint8_t flag = 0x1;
		uint8_t shift = 0;

		while(shift < 4) {
			if (table_chunk.__chunk_present_mask & (flag<<shift)) {
				char *nametest = table_chunk.headers[shift].name;
				if (!strncmp(nametest, name, strlen(nametest))) {
					*entry = shift;
					return i;
				}
			}
			shift++;
		}
	}
	return 0;
}

FS_ERROR sfs_file_delete(fs_t *fs, char *name) {
	uint8_t entry = 0;
	uint64_t hdr_no = get_file_header_by_name(fs, name, &entry);
	if (!hdr_no) {
		return FILE_NOT_FOUND;
	}

	table_chunk_t table_chunk;
	fs->_chunk_read(fs, hdr_no, &table_chunk);
	uint64_t blk_delete = table_chunk.headers[entry].first_chunk;
	table_chunk.__chunk_present_mask ^= (0x1 << entry);
	if (!table_chunk.__chunk_present_mask) {
		table_chunk.__used = 0;
	} else {
		table_chunk.__checksum = checksum(
			(uint8_t *)table_chunk.headers,
			sizeof(table_chunk.headers));
	}
	fs->_chunk_write(fs, hdr_no, &table_chunk);

	while(blk_delete) {
		file_chunk_t delete_me;
		fs->_chunk_read(fs, blk_delete, &delete_me);

		if (is_valid_occupied_file_chunk(delete_me)) {
			delete_me.occupied_bytes = 0;
			delete_me.data_checksum = 0;
			uint64_t old = blk_delete;
			blk_delete = delete_me.next_chunk;
			delete_me.next_chunk = 0;
			fs->_chunk_write(fs, old, &delete_me);
		}
	}
	return NO_ERROR;
}

FS_ERROR sfs_file_read(fs_t *fs, char *name, void *data, uint64_t max_len, uint64_t *read) {
	system_desc_chunk_t system_chunk;
	fs->_chunk_read(fs, 0, &system_chunk);

	if (system_chunk.table_chunk_count == 0) {
		return FILE_NOT_FOUND;
	}

	for(uint64_t i=1; i<=system_chunk.table_chunk_count; i++) {
		table_chunk_t table_chunk;
		fs->_chunk_read(fs, i, &table_chunk);
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
	return FILE_NOT_FOUND;
}

void memory_chunk_read(fs_t *fs, uint64_t chunk_no, void *data) {
	chunk_t *read_from = ((chunk_t *)(fs->_underlying_data))+chunk_no;
	memcpy(data, read_from, CHUNK_SIZE);
}

void memory_chunk_write(fs_t *fs, uint64_t chunk_no, void *data) {
	chunk_t *write_to = ((chunk_t *)(fs->_underlying_data))+chunk_no;
	memcpy(write_to, data, CHUNK_SIZE);
}

void ata_chunk_read(fs_t *fs, uint64_t chunk_no, void *data) {
	ata_device_read_sector(fs->_underlying_data, chunk_no, data);
}

void ata_chunk_write(fs_t *fs, uint64_t chunk_no, void *data) {
	ata_device_write_sector_retry(fs->_underlying_data, chunk_no, data);
}

uint64_t ata_size_bytes(fs_t *fs) {
	struct ata_device *dev = fs->_underlying_data;
	return dev->identity.sectors_48 * ATA_SECTOR_SIZE;
}

bool fs_memory_ID(char *name) {
	return !strncmp(name, "SFS_memory", 11);
}

bool fs_ATA_ID(char *name) {
	return !strncmp(name, "SFS_ATA", 8);
}

uint64_t stupid_size(fs_t *fs) {
	return (uint64_t)fs->_UD2;
}

fs_t get_fs_memory(uint8_t *buffer, uint64_t bytes) {
	fs_t result;
	memset(&result, 0, sizeof(fs_t));
	result._UD2 = (void *)bytes;
	result._underlying_data = buffer;
	result._chunk_read = memory_chunk_read;
	result._chunk_write = memory_chunk_write;
	result.file_read = sfs_file_read;
	result.file_write = sfs_file_write;
	result.file_delete = sfs_file_delete;
	result.get_fs_size_bytes = stupid_size;
	result.identity = fs_memory_ID;
	return result;
}

fs_t get_fs_ata(struct ata_device *dev) {
	fs_t result;
	memset(&result, 0, sizeof(fs_t));
	result._underlying_data = dev;
	result._chunk_read = ata_chunk_read;
	result._chunk_write = ata_chunk_write;
	result.file_read = sfs_file_read;
	result.file_write = sfs_file_write;
	result.file_delete = sfs_file_delete;
	result.get_fs_size_bytes = ata_size_bytes;
	result.identity = fs_ATA_ID;
	return result;
}