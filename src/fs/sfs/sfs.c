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

	inode_t inode;
	inode.data.entry_count = 0;
	uint64_t table;
	uint32_t slot;
	fs->write_new_dir(fs, &table, &slot, &inode);

	if (table!=1 || slot!=0) {
		while(1);
	}
}

uint64_t checksum(uint8_t *data, uint64_t bytes) {
	uint64_t result = 0;
	for(uint64_t i=0; i<bytes; i++) {
		result ^= data[i];
		uint64_t msb = result >> 63;
		result = (result<<1 | msb);
	}
}

uint64_t sfs_get_next_file_entry_chunk_no(fs_t *fs, uint32_t *open_spot) {
	system_desc_chunk_t system_chunk;
	fs->_chunk_read(fs, 0, &system_chunk);

	if (system_chunk.magic_number != magic.number) {
		while(1); //PANIC
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
	if (table_chunk.__chunk_present_mask == 0x1FFFFFF) {
		chunk_no++;
		goto create_new_table;
	}

	uint32_t next_spot = 0;
	while(next_spot < 25) {
		if (!((0x01<<next_spot) & table_chunk.__chunk_present_mask)) {
			*open_spot = next_spot;
			return chunk_no;
		}
	}
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








#define check_read(tc, ent, slot) \
	do { \
		fs->_chunk_read(fs, (ent), &(tc)); \
		uint64_t table_checksum = checksum( \
			(uint8_t *)table_chunk.headers, \
			sizeof(table_chunk.headers)); \
		if (table_chunk.__checksum != table_checksum) { \
			return FILE_NOT_FOUND; \
		} \
		if (!(table_chunk.__chunk_present_mask & (0x1 << slot))) { \
			return FILE_NOT_FOUND; \
		} \
	} while(0)


FS_ERROR sfs_file_delete(fs_t *fs, uint64_t hdr_no, uint32_t entry) {
	table_chunk_t table_chunk;
	check_read(table_chunk, hdr_no, entry);
	if (!(table_chunk.__chunk_present_mask & (0x01 << entry))) {
		return FILE_NOT_FOUND;
	}

	uint64_t blk_delete = table_chunk.headers[entry].first_chunk;
	table_chunk.__chunk_present_mask &= ~(0x1 << entry);
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



/* file property getting functions */
FS_ERROR sfs_get_size(fs_t *fs, uint64_t table_entry, uint32_t slot, uint64_t *size) {
	table_chunk_t table_chunk;
	check_read(table_chunk, table_entry, slot);
	*size = table_chunk.headers[slot].size;
	return NO_ERROR;
}

FS_ERROR sfs_get_type(fs_t *fs, uint64_t table_entry, uint32_t slot, node_type *res) {
	table_chunk_t table_chunk;
	check_read(table_chunk, table_entry, slot);
	*res = table_chunk.headers[slot].type;
	return NO_ERROR;
}


/* directory readers work on top of file readers */
FS_ERROR sfs_read_dir_at(fs_t *fs, uint64_t table_entry, uint32_t slot, inode_t *res) {
	node_type type;
	FS_ERROR get = fs->get_type(fs, table_entry, slot, &type);
	if (get != NO_ERROR) {
		return get;
	}
	if (type != TYPE_DIR) {
		return NOT_A_DIRECTORY;
	}
	res->fs = fs;
	return fs->read_file_at(fs, table_entry, slot, 0, NULL, &res->data);
}

FS_ERROR sfs_write_dir_at(fs_t *fs, uint64_t table_entry, uint32_t slot, inode_t *res) {
	uint64_t size = res->data.entry_count * sizeof(inode_entry_t);
	FS_ERROR ret = fs->write_file_at(fs, table_entry, slot, size, &(res->data));

	table_chunk_t table_chunk;
	check_read(table_chunk, table_entry, slot);
	table_chunk.headers[slot].type = TYPE_DIR;
	fs->_chunk_write(fs, table_entry, &table_chunk);

	return ret;
}

FS_ERROR sfs_write_new_dir(fs_t *fs, uint64_t *table_entry, uint32_t *slot, inode_t *res) {
	uint64_t size = res->data.entry_count * sizeof(inode_entry_t);
	FS_ERROR ret = fs->write_new_file(fs, table_entry, slot, size, &(res->data));

	table_chunk_t table_chunk;
	check_read(table_chunk, *table_entry, *slot);
	table_chunk.headers[*slot].type = TYPE_DIR;
	fs->_chunk_write(fs, *table_entry, &table_chunk);

	return ret;
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

FS_ERROR sfs_read_file_at(fs_t *fs, uint64_t te, uint32_t slot, uint64_t max_or_E, uint64_t *read, void *into) {
	if (max_or_E == 0) {
		max_or_E = ~0x0;
	}
	table_chunk_t table_chunk;
	check_read(table_chunk, te, slot);
	uint64_t read_bytes = sfs_read_file_data(fs, table_chunk.headers[slot], into, max_or_E);
	if (read_bytes) {
		if (read) {
			*read = read_bytes;
		}
		return NO_ERROR;
	}
	return ERROR_CRIT;
}

FS_ERROR sfs_write_file_at(fs_t *fs, uint64_t te, uint32_t slot, uint64_t len, void *data) {
	fs->try_delete_at(fs, te, slot);

	system_desc_chunk_t system_chunk;
	fs->_chunk_read(fs, 0, &system_chunk);
	uint64_t file_chunks_needed = (len + CHUNK_SIZE - 17) / (CHUNK_SIZE - 16);
	uint64_t free_chunks = system_chunk.total_chunk_count - 
			(system_chunk.table_chunk_count + system_chunk.file_chunk_count);

	if (free_chunks < file_chunks_needed) {
		return NOT_ENOUGH_SPACE;
	}

	system_chunk.file_chunk_count += file_chunks_needed;
	uint64_t backup_len = len;

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
	fs->_chunk_read(fs, te, &table_chunk);

	table_chunk.headers[slot].ZEROES = 0;
	table_chunk.headers[slot].rwx_own = 7;
	table_chunk.headers[slot].rwx_grp = 7;
	table_chunk.headers[slot].rwx_all = 7;
	table_chunk.headers[slot].type = TYPE_FILE;
	table_chunk.headers[slot].size = backup_len;
	table_chunk.headers[slot].first_chunk = first_chunk_no;
	table_chunk.__chunk_present_mask |= 0x1<<slot;
	fs->_chunk_write(fs, 0, &system_chunk);
	fs->_chunk_write(fs, te, &table_chunk);

	return NO_ERROR;
}

FS_ERROR sfs_write_new_file(fs_t *fs, uint64_t *te, uint32_t *slot, uint64_t len, void *from) {
	if (len == 0) {
		return NO_ACTION;
	}

	uint32_t __slot = 0;
	uint64_t __te = sfs_get_next_file_entry_chunk_no(fs, &__slot);
	
	FS_ERROR res = fs->write_file_at(fs, __te, __slot, len, from);
	if (res == NO_ERROR) {
		*te = __te;
		*slot = __slot;
		return res;
	}
	return res;
}





/* chunk functions for ata & memory */
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


/* get size of filesystem */
uint64_t ata_size_bytes(fs_t *fs) {
	struct ata_device *dev = fs->_underlying_data;
	return dev->identity.sectors_48 * ATA_SECTOR_SIZE;
}

uint64_t stupid_size(fs_t *fs) {
	return (uint64_t)fs->_UD2;
}

/* identify filesystems */
bool fs_memory_ID(char *name) {
	return !strncmp(name, "SFS_memory", 11);
}

bool fs_ATA_ID(char *name) {
	return !strncmp(name, "SFS_ATA", 8);
}


fs_t get_fs_memory(uint8_t *buffer, uint64_t bytes) {
	fs_t result;
	memset(&result, 0, sizeof(fs_t));

	/* setup private data */
	result._UD2 = (void *)bytes;
	result._underlying_data = buffer;
	result._chunk_read = memory_chunk_read;
	result._chunk_write = memory_chunk_write;

	/* memory specific functions */
	result.get_fs_size_bytes = stupid_size;
	result.identity = fs_memory_ID;

	/* sfs specific functions */
	result.try_delete_at = sfs_file_delete;
	result.get_type = sfs_get_type;
	result.get_size = sfs_get_size;
	result.read_dir_at = sfs_read_dir_at;
	result.write_dir_at = sfs_write_dir_at;
	result.read_file_at = sfs_read_file_at;
	result.write_file_at = sfs_write_file_at;

	return result;
}

fs_t get_fs_ata(struct ata_device *dev) {
	fs_t result;
	memset(&result, 0, sizeof(fs_t));

	/* setup private data */
	result._UD2 = NULL;
	result._underlying_data = dev;
	result._chunk_read = ata_chunk_read;
	result._chunk_write = ata_chunk_write;

	/* memory specific functions */
	result.get_fs_size_bytes = ata_size_bytes;
	result.identity = fs_ATA_ID;

	/* sfs specific functions */
	result.try_delete_at = sfs_file_delete;
	result.get_type = sfs_get_type;
	result.get_size = sfs_get_size;
	result.read_dir_at = sfs_read_dir_at;
	result.write_dir_at = sfs_write_dir_at;
	result.read_file_at = sfs_read_file_at;
	result.write_file_at = sfs_write_file_at;
	return result;
}