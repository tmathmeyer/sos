#ifndef multiboot_h
#define multiboot_h

#include <std/int.h>

#define ELF_TAG_TYPE 0x09
#define MMAP_TAG_TYPE 0x06

struct tag {
    uint16_t type;
    uint16_t flags;
    uint32_t size;
};

struct multiboot_header {
    uint32_t total_size;
    uint32_t _reserved;
    struct tag tags;
};

struct multiboot_tag_iterator {
    struct tag *current;
};

struct memory_area {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t _reserved;
};

struct memory_map_tag {
    uint32_t typ;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct memory_area first_area;
};

struct elf_section {
    uint32_t _flags;
    uint32_t type;
    uint32_t _addr;
    uint32_t addr;
    uint32_t _size;
    uint32_t offset;
    uint32_t flags;
    uint32_t size;
    uint64_t link;
    uint32_t info;
    uint32_t alignment;
    uint64_t name;
    uint64_t entry_size;
};

struct elf_section_tag {
    uint32_t type;
    uint32_t size;
    uint32_t number_sections;
    uint32_t entry_size;
    uint32_t string_table;
    struct elf_section first_section;
};

typedef struct {
    uint64_t start;
    uint64_t end;
} bounds_t;




int valid_multiboot(struct multiboot_header *);
struct multiboot_tag_iterator get_tag_iterator(struct multiboot_header *);
void *find_by_type(struct multiboot_header *, unsigned short);
void itr_memory_mapped_locations(struct memory_map_tag *mmt, void (*do_with_area)(struct memory_area *));
void itr_elf_sections(struct elf_section_tag *mmt, void (*do_with_area)(struct elf_section *));
bounds_t get_large_ram_area(struct memory_map_tag *);




#endif
