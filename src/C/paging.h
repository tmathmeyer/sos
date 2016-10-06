#ifndef paging_h
#define paging_h

#include "multiboot.h"

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 512
#define PAGE_TABLE4 0xfffffffffffff000

typedef uint64_t page_t;
typedef uint64_t frame_t;
typedef void *virtual_address;
typedef uint64_t physical_address;

typedef struct {
    union {
        uint64_t packed;
        struct { uint8_t present : 1;
            uint8_t writable : 1;
            uint8_t user_accessable : 1;
            uint8_t write_thru_cache : 1;
            uint8_t disable_cache : 1;
            uint8_t accessed : 1;
            uint8_t dirty : 1;
            uint8_t huge_page : 1;
            uint8_t global : 1;
            uint8_t OS_1 : 1;
            uint8_t OS_2 : 1;
            uint8_t OS_3 : 1;
            uint64_t _addr_mask : 40;
            uint8_t OS_4 : 1;
            uint8_t OS_5 : 1;
            uint8_t OS_6 : 1;
            uint8_t OS_7 : 1;
            uint8_t OS_8 : 1;
            uint8_t OS_9 : 1;
            uint8_t OS_A : 1;
            uint8_t OS_B : 1;
            uint8_t OS_C : 1;
            uint8_t OS_D : 1;
            uint8_t OS_E : 1;
            uint8_t no_execute : 1;
        };
    };
}__attribute__((packed)) page_entry_t;
typedef page_entry_t *page_table_t;

typedef struct {
    uint64_t current_frame_index;
    struct memory_area *area;
    struct memory_map_tag *mem_info;
    physical_address kernel_start;
    physical_address kernel_end;
    physical_address mboot_start;
    physical_address mboot_end;
} frame_allocator;


// 256 byte structure!
// we can fit exactly 16 of these per page
// because of that, each pointer to a FAT_list
// is aligned such that there are 16 free bytes
// in both .next and .prev
// this serves as storage and sanity checksumming,
// as the first 16 bits of each must match.
// bit 0 of each determines if this is a free or allocated block
typedef struct FAT_list {
    frame_t start;
    frame_t end;
    struct FAT_list *next;
    struct FAT_list *prev;
} FAT_list_t;

#define FAT_POINTER(ADDR) (FAT_list_t *)(((uint64_t)(ADDR))&0xFFFFFFFFFFFFFFF0)
#define FAT_STATUS(ADDR) (((uint64_t)(ADDR))&0x000000000000000F)
#define FAT_CHECKSUM(A, B) (FAT_STATUS(A)==FAT_STATUS(B))
#define FAT_SET_STATUS(ADDR, s) ((ADDR) = FAT_POINTER(ADDR)|(s&0x000000000000000F))

page_t allocate_page();
frame_t late_allocate_frame();
void print_frame_alloc_table_list_entry(uint64_t);
FAT_list_t *vpage_allocator(frame_allocator *fa);
frame_t allocate_frame();
uint64_t containing_address(uint64_t addr);
void unmap_page(page_t, frame_allocator *);
uint64_t starting_address(uint64_t page_frame);
page_table_t referenced_table(page_entry_t entry);
frame_t map_page(page_t, uint8_t, frame_allocator *);
physical_address translate_address(virtual_address);
void identity_map(frame_t, uint8_t, frame_allocator *);
void map_page_to_frame(page_t, frame_t, uint8_t, frame_allocator *);
void show_page_table_layout_for_address(uint64_t address);

frame_allocator init_allocator(struct memory_map_tag *, physical_address,
                                                        physical_address,
                                                        physical_address,
                                                        physical_address);
#endif

