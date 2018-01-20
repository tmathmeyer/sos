/*
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "fat32.h"

void get_fat_info(fat_bootsector *fat_boot, fat_type *type, unsigned int *root_sector, unsigned int *fds) {
	unsigned long total_sectors = (fat_boot->total_sectors_16 == 0)? fat_boot->total_sectors_32 : fat_boot->total_sectors_16;
	unsigned long fat_size = (fat_boot->table_size_16 == 0)? fat_boot->fat32.table_size_32 : fat_boot->table_size_16;
	unsigned long root_dir_sectors = ((fat_boot->root_entry_count * 32) + (fat_boot->bytes_per_sector - 1)) / fat_boot->bytes_per_sector;
	unsigned int first_data_sector = fat_boot->reserved_sector_count + (fat_boot->table_count * fat_size) + root_dir_sectors;
	unsigned int first_fat_sector = fat_boot->reserved_sector_count;
	unsigned int data_sectors = total_sectors - (fat_boot->reserved_sector_count + (fat_boot->table_count * fat_size) + root_dir_sectors);
	unsigned int total_clusters = data_sectors / fat_boot->sectors_per_cluster;

	if(total_clusters < 4085) {
		*type = FAT12;
		*root_sector = first_data_sector - root_dir_sectors;
	} 
	else if(total_clusters < 65525) {
		*type = FAT16;
		*root_sector = first_data_sector - root_dir_sectors;
	} 
	else if (total_clusters < 268435445) {
		*type = FAT32;
		*root_sector = fat_boot->fat32.root_cluster;
	}
	else {
		*type = ExFAT;
		*root_sector = 0;
		return;
	}
	*fds = first_data_sector;
	*root_sector = ((*root_sector - 2) * fat_boot->sectors_per_cluster) + first_data_sector;
}
















sector_buffer create_buffer(void *data) {
	sector_buffer res;
	res.data = data;
	res.index = 0;
	return res;
}

void bread(sector_buffer *buf, void *_dest, unsigned long len) {
	unsigned char *dest = _dest;
	while(len) {
		*(dest++) = buf->data[buf->index++];
		len--;
	}
}
void blookahead(sector_buffer *buf, void *_dest, unsigned long len) {
	unsigned char *dest = _dest;
	unsigned int index = buf->index;
	while(len) {
		*(dest++) = buf->data[index++];
		len--;
	}
}

void parse_headers(sector_buffer *buf, char *filename, fat_directory_entry *dir) {
	long_name_entry _entry;
	long_name_entry *entry = &_entry;
	unsigned char entry_number = 100;

	while(entry_number > 1) {
		bread(buf, entry, sizeof(_entry));
		entry_number = (entry->entry_order)&0xF;
		unsigned char offset = (entry_number - 1)*13;

		for(int i=0; i<5; i++) {
			unsigned char c = entry->first_chars[i].byte_1;
			if (c==0xFF || c==0x00) break;
			else filename[offset++] = c;
		}
		for(int i=0; i<6; i++) {
			unsigned char c = entry->second_chars[i].byte_1;
			if (c==0xFF || c==0x00) break;
			else filename[offset++] = c;
		}
		for(int i=0; i<2; i++) {
			unsigned char c = entry->third_chars[i].byte_1;
			if (c==0xFF || c==0x00) break;
			else filename[offset++] = c;
		}
	}
	bread(buf, dir, sizeof(fat_directory_entry));
}


int _iterate_dir_entries(sector_buffer *buf, char **filename, fat_directory_entry *dir_e, void *(*alloc)(size_t)) {
	unsigned char entry[32];
try_again:
	blookahead(buf, entry, sizeof(entry));
	if (entry[0] == 0x00) { // no entry here
		return 0;
	}
	if (entry[0] == 0xE5) { // unused entry
		buf->index += sizeof(entry);
		goto try_again;
	}
	if (entry[11] == 0x0F) { // long name entry
		*filename = alloc((entry[0]&0x0F) * 13);
		parse_headers(buf, *filename, dir_e);
		return 1;
	}
	bread(buf, dir_e, sizeof(fat_directory_entry));
	*filename = alloc(12);
	memcpy(*filename, dir_e->file_name, 11);
	for(int i=10; i; i--) {
		if ((*filename)[i] != ' ') {
			break;
		}
		(*filename)[i] = 0;
	}
	return 1;
}

directory_ll *iterate_dir_entries(sector_buffer *buf, void *(*alloc)(size_t), fat_bootsector *bootsector) {
	directory_ll *tail = NULL;
	char *name = NULL;
	fat_directory_entry dir;
	while(_iterate_dir_entries(buf, &name, &dir, alloc)) {
		directory_ll *next = alloc(sizeof(directory_ll));
		next->next = tail;
		next->name = name;
		next->entry = dir;

		unsigned int entry_cluster = 
			dir.entry_cluster_high_bits[1] << 24 |
			dir.entry_cluster_high_bits[0] << 16 |
			dir.entry_cluster_low_bits[1]  << 8  |
			dir.entry_cluster_low_bits[0];
		next->file_sector_start = get_sector_offset(entry_cluster & 0x0FFFFFFF, bootsector);
		
		tail = next;
	}
	return tail;
}

unsigned long get_sector_offset(unsigned int sector, fat_bootsector *bs) {
	unsigned long total_sectors = (bs->total_sectors_16 == 0)? bs->total_sectors_32 : bs->total_sectors_16;
	unsigned long fat_size = (bs->table_size_16 == 0)? bs->fat32.table_size_32 : bs->table_size_16;
	unsigned long root_dir_sectors = ((bs->root_entry_count * 32) + (bs->bytes_per_sector - 1)) / bs->bytes_per_sector;
	unsigned int first_data_sector = bs->reserved_sector_count + (bs->table_count * fat_size) + root_dir_sectors;
	return (((sector - 2) * bs->sectors_per_cluster) + first_data_sector);
}

int is_directory(fat_directory_entry entry) {
	return entry.file_attributes & 0x10;
}

void tree(unsigned int indent, unsigned int dirsector, sector_buffer buf, fat_bootsector bootsector) {
	buf.index = dirsector * 512;
	directory_ll *entries = iterate_dir_entries(&buf, malloc, &bootsector);

	while(entries) {
		for(int i=0; i<indent; i++) {
			printf("  ");
		}
		if (is_directory(entries->entry)) {
			printf("%s", entries->name);
			if (!strncmp(entries->name, "..", 3)) {
				printf(" (parent)\n");
			} else if (!strncmp(entries->name, ".", 2)) {
				printf(" (self)\n");
			} else {
				printf(":\n");
				tree(indent+1, entries->file_sector_start, buf, bootsector);
			}
		} else {
			printf("%s[%i] size = %i\n",
				entries->name,
				entries->file_sector_start,
				entries->entry.file_size);
		}
		entries = entries->next;
		for(int i = 0; i<100000000; i++);
	}
}

int main(int argc, char **argv) {
	int fd = open(argv[1], O_RDONLY);
	void *__buf = malloc(1024*1024*100);
	read(fd, __buf, 1024*1024*100);
	sector_buffer buf = create_buffer(__buf);



	fat_bootsector bootsector;
	fat_type type;
	unsigned int root_sector;
	unsigned int first_data_sector;
	fat_directory_entry entry;



	bread(&buf, &bootsector, sizeof(fat_bootsector));
	if (bootsector.token != 0xAA55) {
		return 1;
	}
	get_fat_info(&bootsector, &type, &root_sector, &first_data_sector);


	tree(0, root_sector, buf, bootsector);
}
*/