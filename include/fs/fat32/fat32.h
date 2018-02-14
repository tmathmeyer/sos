#ifndef fat32_h
#define fat32_h

#include <std/int.h>

typedef
struct {
	unsigned char 		bootjmp[3];
	unsigned char 		oem_name[8];
	unsigned short		bytes_per_sector;
	unsigned char		sectors_per_cluster;
	unsigned short		reserved_sector_count;
	unsigned char		table_count;
	unsigned short		root_entry_count;
	unsigned short		total_sectors_16;
	unsigned char		media_type;
	unsigned short		table_size_16;
	unsigned short		sectors_per_track;
	unsigned short		head_side_count;
	unsigned int 		hidden_sector_count;
	unsigned int 		total_sectors_32;

	union {
		struct {
			unsigned char		bios_drive_num;
			unsigned char		reserved1;
			unsigned char		boot_signature;
			unsigned int		volume_id;
			unsigned char		volume_label[11];
			unsigned char		fat_type_label[8];
		}__attribute__((packed)) fat16;
		struct {
			unsigned int		table_size_32;
			unsigned short		extended_flags;
			unsigned short		fat_version;
			unsigned int		root_cluster;
			unsigned short		fat_info;
			unsigned short		backup_BS_sector;
			unsigned char 		reserved_0[12];
			unsigned char		drive_number;
			unsigned char 		reserved_1;
			unsigned char		boot_signature;
			unsigned int 		volume_id;
			unsigned char		volume_label[11];
			unsigned char		fat_type_label[8];
		}__attribute__((packed)) fat32;
	};
	unsigned char padding [510-90];
	unsigned short token;
}__attribute__((packed)) fat_bootsector;

typedef
enum {
	FAT12,
	FAT16,
	FAT32,
	ExFAT
} fat_type;

typedef
struct {
	unsigned char	year: 7;
	unsigned char	month: 4;
	unsigned char	day: 5;
} fat_date_t;

typedef
struct {
	unsigned char	hour: 5;
	unsigned char	minutes: 6;
	unsigned char	seconds: 5;
} fat_time_t;

typedef
struct {
	unsigned char	file_name[11];
	unsigned char	file_attributes;
	unsigned char	reserved;
	unsigned char	creat_time;
	unsigned short	create_time;
	unsigned short	create_date;
	unsigned short	access_date;
	unsigned char	entry_cluster_high_bits[2];
	unsigned short	modification_time;
	unsigned short	modification_date;
	unsigned char	entry_cluster_low_bits[2];
	unsigned int	file_size;
}__attribute__((packed)) fat_directory_entry;

typedef
struct {
	unsigned char byte_1;
	unsigned char byte_2;
} two_byte_char;

typedef
struct {
	unsigned char	entry_order;
	two_byte_char	first_chars[5];
	unsigned char	attribute;
	unsigned char	logn_entry_type;
	unsigned char	checksum;
	two_byte_char	second_chars[6];
	unsigned short	always_zero;
	two_byte_char	third_chars[2];
}__attribute__((packed)) long_name_entry;

#endif