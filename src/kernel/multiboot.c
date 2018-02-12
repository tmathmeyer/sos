#include <std/int.h>
#include <kernel/multiboot.h>

int valid_multiboot(struct multiboot_header *header) {
	const struct tag END = {.type=0, .flags=0, .size=8};

	uint64_t end_addr = (uint64_t) header;
	end_addr += (header->total_size - END.size);

    struct tag *end_tag = (struct tag *)end_addr;

    if (end_tag->type != END.type) {
        return 'T';
    }

    if (end_tag->size != END.size) {
        return 'S';
    }

    return 0;
}

struct multiboot_tag_iterator get_tag_iterator(struct multiboot_header *header) {
    struct multiboot_tag_iterator result = {.current=&(header->tags)};
    return result;
}

void *find_by_type(struct multiboot_header *header, uint16_t type) {
    struct tag *current = &(header->tags);
    while(current->type != type) {

        if (current->type==0 && current->size==8) {
            return 0;
        }

        uint64_t tag_addr = (uint64_t)current;
        tag_addr += current->size;
        tag_addr = ((tag_addr-1) & ~0x7) + 8;
        current = (struct tag *)tag_addr;
    }

    return current;
}


void itr_memory_mapped_locations(struct memory_map_tag *mmt, void (*do_with_area)(struct memory_area *)) {
    struct memory_area *current = (struct memory_area *)&(mmt->first_area);
    struct memory_area *last_area = (struct memory_area *)(
            (uint64_t)mmt +
            mmt->size -
            mmt->entry_size);
    uint32_t entry_size = mmt->entry_size;

    while(((uint64_t)current) <= ((uint64_t)last_area)) {
        if (current->type == 1) {
            do_with_area(current);
        }
        current = (struct memory_area *)(((uint64_t)current) + entry_size);
    }
}

void itr_elf_sections(struct elf_section_tag *est, void (*do_with_section)(struct elf_section *)) {
    struct elf_section *current = &(est->first_section);
    uint32_t remaining_sections = est->number_sections - 1;
    uint32_t entry_size = est->entry_size;

    while(remaining_sections > 0) {
        struct elf_section *section = current;
        uint64_t next_addr = (uint64_t)current + entry_size;
        current = (struct elf_section *)next_addr;
        remaining_sections--;
        if (section->type != 0) {
            do_with_section(section);
        }
    }
}

bounds_t get_large_ram_area(struct memory_map_tag *mmt) {
    uint64_t mm_start = 0;
    uint64_t mm_end = 0;

    void set_last_mm(struct memory_area *area) {
        mm_start = area->base_addr;
        mm_end = mm_start+area->length;
    }

    itr_memory_mapped_locations(mmt, set_last_mm);
    bounds_t result;
    result.start = mm_start;
    result.end = mm_end;
    return result;
}
