#include <libk.h>
#include <reefs.h>
#include <fs.h>
#include <kio.h>
#include <path.h>


uint64_t checksum(table_chunk_t t) {
    uint64_t res = 0x1234567812345678;
    uint64_t cpm = t.chunk_present_mask;
    res ^= cpm;
    res ^= cpm<<32;
    res ^= cpm<<16;
    return res;
}

uint64_t file_checksum(file_chunk_t data) {
    uint64_t result = 0x1239871993892;
    return result;
}

int valid_checksum(table_chunk_t t) {
    return checksum(t) == t.checksum;
}

int is_dir(file_header_t t) {
    return t.type = FTYPE_DIR;
}

uint64_t find_free_entry(uint32_t t) {
    uint32_t X = 0x1;
    for(uint64_t i=0; i<32; i++) {
        if (!((X<<i)&t)) {
            return i;
        }
    }
    return 99999;
}

uint64_t reefs_next_free_chunk(fs_disk_t *disk) {
    header_chunk_t header;

    if (!disk->read_offset(disk, 0, &header, sizeof(chunk_t))) {
        return DISK_WRITE_ERROR;
    }
    uint64_t result = 0;
    file_chunk_t t;
    for (uint64_t i=header.total_chunk_count-1; i>header.table_chunk_count+1; i--) {
        disk->read_offset(disk, i*sizeof(chunk_t), &t, sizeof(chunk_t));
        if (t.data_checksum != file_checksum(t)) {
            result = i;
            goto out;
        }
    }
    return 0;

out:
    header.file_chunk_count ++;
    disk->write_offset(disk, 0, &header, sizeof(chunk_t));
    t.data_checksum = file_checksum(t);
    disk->write_offset(disk, result*sizeof(chunk_t), &t, sizeof(chunk_t));
    return result;
}

void reefs_file_close(fs_file_t *file) {
    file->close = NULL;
    file->rewind = NULL;
    file->read = NULL;
    file->write = NULL;

    file_header_t header;
    uint64_t loc[2];
    memcpy(&header, file->_data+16, 16);
    memcpy(loc, file->_data+32, 16);

    table_chunk_t t;
    fs_disk_t *disk = file->_fs->_disk;
    disk->read_offset(disk, (loc[0]+1)*sizeof(chunk_t), &t, sizeof(chunk_t));
    memcpy(&t.headers[loc[1]], &header, sizeof(file_header_t));
    disk->write_offset(disk, (loc[0]+1)*sizeof(chunk_t), &t, sizeof(chunk_t));
}

void reefs_file_rewind(fs_file_t *file) {
    file->_position = 0;
    memcpy(file->_data, file->_data+16, 16);
}

void reefs_file_seekend(fs_file_t *file) {
    char trash[128];
    reefs_file_rewind(file);
    while(file->read(file, trash, 128));
}

uint64_t reefs_file_read(fs_file_t *file, void *dest, uint64_t len) {
    fs_disk_t *disk = file->_fs->_disk;
    file_header_t file_header;
    memcpy(&file_header, file->_data, sizeof(file_header_t));

    if (!(file_header.rwx & 0x04)) {
        return 0;
    }

    if (!file_header.size) {
        return 0;
    }

    uint64_t dest_offset = 0;
    uint8_t *_dest = dest;
    while (1) {
        file_chunk_t file_chunk;
        disk->read_offset(
                disk,
                file_header.first_chunk*sizeof(chunk_t),
                &file_chunk,
                sizeof(file_chunk_t)
                );
        uint64_t copy_amount = min(len, file_chunk.occupied_bytes-(file->_position));

        memcpy(&_dest[dest_offset], file_chunk.data+file->_position, copy_amount);
        len -= copy_amount;
        dest_offset += copy_amount;
        file->_position += copy_amount;
        if (!len || (!file_chunk.next_chunk && file->_position==file_chunk.occupied_bytes)) {
            return dest_offset;
        }
        file->_position = 0;
        file_header.first_chunk = file_chunk.next_chunk;
    }
}

uint64_t reefs_file_write(fs_file_t *file, void *src, uint64_t len) {
    fs_disk_t *disk = file->_fs->_disk;
    file_header_t header;
    memcpy(&header, file->_data, 16);

    file_chunk_t chunk;
    disk->read_offset(disk, header.first_chunk*sizeof(chunk_t), &chunk, sizeof(chunk_t));

    uint64_t size = 0;
    char *SRC = src;
    uint64_t cur_chunk_addr = header.first_chunk;

    while(len > 496-file->_position) {
        memcpy(chunk.data+file->_position, src, 496-file->_position);
        SRC += (496-file->_position);
        len -= (496-file->_position);
        chunk.occupied_bytes = 496;
        size += 496;
        chunk.data_checksum = file_checksum(chunk);
        chunk.next_chunk = reefs_next_free_chunk(disk);
        disk->write_offset(disk, cur_chunk_addr*sizeof(chunk_t), &chunk, sizeof(chunk_t));

        cur_chunk_addr = chunk.next_chunk;
        disk->read_offset(disk, cur_chunk_addr*sizeof(chunk_t), &chunk, sizeof(chunk_t));
        chunk.occupied_bytes = 0;
        chunk.next_chunk = 0;
        file->_position = 0;
    }

    if (file->_position+len < 496) {
        memcpy(&chunk.data[file->_position], src, len);
        file->_position += len;
        chunk.occupied_bytes = max(chunk.occupied_bytes, file->_position);
        size += chunk.occupied_bytes;
        chunk.data_checksum = file_checksum(chunk);
        disk->write_offset(disk, cur_chunk_addr*sizeof(chunk_t), &chunk, sizeof(chunk_t));
    }

    memcpy(&header, file->_data+16, 16);
    header.size = size;
    memcpy(file->_data+16, &header, 16);

    return 0;
}

FS_ERROR reefs_open_raw(fs_t *fs, uint64_t loc[2], fs_file_t *entry) {
    fs_disk_t *disk = fs->_disk;
    table_chunk_t table;
    disk->read_offset(disk, (loc[0]+1)*sizeof(chunk_t), &table, sizeof(chunk_t));
    if (!valid_checksum(table)) {
        return FILE_NOT_FOUND;
    }
    file_header_t header = table.headers[loc[1]];
    entry->close = reefs_file_close;
    entry->rewind = reefs_file_rewind;
    entry->seekend = reefs_file_seekend;
    entry->read = reefs_file_read;
    entry->write = reefs_file_write;
    entry->_fs = fs;
    entry->_position = 0;
    memcpy(entry->_data, &header, sizeof(file_header_t));
    memcpy(entry->_data+16, &header, sizeof(file_header_t));
    memcpy(entry->_data+32, loc, sizeof(uint64_t)*2);
    return NO_ERROR;
}

FS_ERROR reefs_new_file(fs_t *fs, char *path, fs_file_t *entry, fs_file_t *dir, FLAGS flags) {
    fs_disk_t *disk = fs->_disk;
    header_chunk_t header;
    disk->read_offset(disk, 0, &header, sizeof(chunk_t));

    for(uint64_t i=0; i<header.table_chunk_count+1; i++) {
        table_chunk_t table;
        disk->read_offset(disk, (i+1)*sizeof(chunk_t), &table, sizeof(chunk_t));
        if (i == header.table_chunk_count) {
            table.in_use = 1;
            table.chunk_present_mask = 0;
        }
        if (table.chunk_present_mask ^ 0xFFFFFFFF) {
            uint64_t free_entry = find_free_entry(table.chunk_present_mask);
            table.chunk_present_mask |= (0x1 << free_entry);
            table.headers[free_entry].rwx = 0x7;
            table.headers[free_entry].size = 0;
            if (flags & F_OPT_TYPE_MOUNT) {
                table.headers[free_entry].type = FTYPE_FILE;
                table.headers[free_entry].first_chunk = 0;
            } else {
                if (flags & F_OPT_TYPE_DIR) {
                    table.headers[free_entry].type = FTYPE_DIR;
                } else if (flags & F_OPT_TYPE_FILE) {
                    table.headers[free_entry].type = FTYPE_FILE;
                }
                table.headers[free_entry].first_chunk = reefs_next_free_chunk(disk);
            }

            entry->_fs = fs;
            entry->_position = 0;
            memcpy(entry->_data, &table.headers[free_entry], sizeof(file_header_t));
            memcpy(entry->_data+16, &table.headers[free_entry], sizeof(file_header_t));
            memcpy(entry->_data+32, &i, sizeof(uint64_t));
            memcpy(entry->_data+40, &free_entry, sizeof(uint64_t));
            entry->close = reefs_file_close;
            entry->rewind = reefs_file_rewind;
            entry->seekend = reefs_file_seekend;
            entry->read = reefs_file_read;
            entry->write = reefs_file_write;

            table.checksum = checksum(table);
            disk->write_offset(disk, (i+1)*sizeof(chunk_t), &table, sizeof(chunk_t));

            dirent_t ent;
            ent._table_block = i+1;
            ent._table_entry = free_entry;
            ent.name_length = strlen(path)+1;

            dir->rewind(dir);
            uint64_t i = 0;
            dir->read(dir, &i, sizeof(uint64_t));
            i++;
            dir->rewind(dir);
            dir->write(dir, &i, sizeof(uint64_t));
            dir->seekend(dir);
            dir->write(dir, &ent, sizeof(ent));
            dir->write(dir, path, strlen(path)+1);
            dir->close(dir);
            return NO_ERROR;
        }
    }
}

int get_location_by_name(fs_file_t *directory, char *seeking, uint64_t *loc) {
    file_header_t header;
    memcpy(&header, directory->_data, 16);
    if (header.size < 8) {
        return 0;
    }

    directory_t dir;
    directory->read(directory, &dir, sizeof(dir));
    for(int i=0; i<dir.entry_count; i++) {
        dirent_t ent;
        directory->read(directory, &ent, sizeof(ent));
        char name[ent.name_length];
        directory->read(directory, name, ent.name_length);
        if (!strncmp(name, seeking, ent.name_length)) {
            loc[0] = ent._table_block;
            loc[1] = ent._table_entry;
            return 1;
        }
    }
    return 0;
}

FS_ERROR reefs_open(fs_t *fs, char *path, fs_file_t *entry, FLAGS flags) {
    fs_file_t dir;
    uint64_t loc[2] = {0};
    FS_ERROR err = reefs_open_raw(fs, loc, &dir);
    if (err) {
        return err;
    }

    char *part;
    file_header_t header;
    mountfile_t mount;
    char *part_start;
    uint64_t remaining = 0;
    path_iterate(path, part, remaining) {
        memcpy(&header, &dir._data, sizeof(file_header_t));

        if (is_dir(header)) {
            if (!get_location_by_name(&dir, part, loc)) {
                err = FILE_NOT_FOUND;
                goto_safe(check_flags_err);
            }
            loc[0]-= 1;
            err = reefs_open_raw(fs, loc, &dir);
            if (err) {
                goto_safe(check_flags_err);
            }
        } else {
            err = NOT_A_DIRECTORY;
            goto_safe(check_flags_err);
        }

    }
    *entry = dir;
    return NO_ERROR;

check_flags_err:
    if (flags&F_OPT_CREATE && err==FILE_NOT_FOUND && remaining==0) {
        return reefs_new_file(fs, part, entry, &dir, flags);
    }
    return err;
}

FS_ERROR reefs_stat(fs_t *fs, char *path, fs_metadata_t *metadata) {
    fs_file_t file;
    FS_ERROR err = reefs_open(fs, path, &file, 0);
    if (err) {
        return err;
    }
    file_header_t header;
    memcpy(&header, &file._data, sizeof(file_header_t));

    metadata->rwx = header.rwx;
    metadata->type = header.type;
    metadata->size_bytes = header.size;
    metadata->UTC_touched = 0;
    metadata->UTC_created = 0;
    return NO_ERROR;
}

FS_ERROR reefs_remove(fs_t *fs, char *path) {
    return FILE_NOT_FOUND;
}

char *reefs_identify(void) {
    return "reefs";
}

FS_ERROR reefs_mkfs(fs_t *fs, fs_disk_t *disk) {
    header_chunk_t header;
    header.magic_number = MAGIC;
    header.version = 1;
    header.table_chunk_count = 1;
    header.file_chunk_count = 0;
    header.total_chunk_count = disk->get_size(disk);

    if (!disk->write_offset(disk, 0, &header, sizeof(chunk_t))) {
        return DISK_WRITE_ERROR;
    }
    fs->_disk = disk;

    fs->mkfs = NULL;
    fs->identify = reefs_identify;
    fs->open = reefs_open;
    fs->stat = reefs_stat;
    fs->remove = reefs_remove;


    struct root_dir {
        uint64_t entry_count;
        uint64_t self_table_block_A;
        uint64_t self_table_entry_A;
        uint64_t self_entry_len_A;
        uint8_t  self_name_A[3];
        uint64_t self_table_block_B;
        uint64_t self_table_entry_B;
        uint64_t self_entry_len_B;
        uint8_t  self_name_B[2];
    }__attribute__((packed));

    table_chunk_t first_table;
    first_table.chunk_present_mask = 0x01;
    first_table.checksum = checksum(first_table);
    first_table.headers[0].rwx = 0x7;
    first_table.headers[0].type = FTYPE_DIR;
    first_table.headers[0].size = sizeof(struct root_dir);
    first_table.headers[0].first_chunk = reefs_next_free_chunk(disk);
    disk->write_offset(disk, 1*sizeof(chunk_t), &first_table, sizeof(chunk_t));

    struct root_dir self = {
        .entry_count = 2,
        .self_table_block_A = 1,
        .self_table_entry_A = 0,
        .self_table_block_B = 1,
        .self_table_entry_B = 0,
        .self_entry_len_A = 3,
        .self_entry_len_B = 2,
        .self_name_A = "..",
        .self_name_B = "."
    };

    file_chunk_t t;
    t.occupied_bytes = sizeof(self);
    t.next_chunk = 0;
    t.zeroes = 0;
    memset(t.data, 0, sizeof(t.data));
    memcpy(t.data, &self, sizeof(self));
    t.data_checksum = file_checksum(t);

    disk->write_offset(disk, first_table.headers[0].first_chunk * sizeof(chunk_t), &t, sizeof(t));
    return NO_ERROR;
}

fs_t empty_fs(void) {
    fs_t result;
    result.identify = NULL;
    result.open = NULL;
    result.stat = NULL;
    result.remove = NULL;
    result.mkfs = reefs_mkfs;
    return result;
}
