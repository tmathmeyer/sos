
#include "ktype.h"
#include "paging.h"

uint64_t containing_address(uint64_t addr) {
    return addr / PAGE_SIZE;
}

uint64_t starting_address(uint64_t page_frame) {
    return page_frame * PAGE_SIZE;
}

page_table_t referenced_table(page_entry_t entry) {
    if (entry.present) {
        uint64_t _entry = entry.packed;
        _entry &= 0x000ffffffffff000;
        return (page_table_t)_entry;
    }
    return 0;
}

page_table_t sub_table_address(page_table_t table, uint64_t at_index) {
    if (table[at_index].present && !table[at_index].huge_page) {
        uint64_t table_address = (uint64_t)table;
        table_address <<= 9;
        table_address |= (at_index << 12);
        return (page_table_t)table_address;
    } else if (table[at_index].huge_page) {
		kprintf("looking at page table : %43x\n", table);
		WARN("uh oh you hit a big page!");
	}
    return (page_table_t)-1;
}

uint64_t p4_index(page_t page) {
    return (((uint64_t)page)>>27) & 0777;
}

uint64_t p3_index(page_t page) {
    return (((uint64_t)page)>>18) & 0777;
}

uint64_t p2_index(page_t page) {
    return (((uint64_t)page)>>9) & 0777;
}

uint64_t p1_index(page_t page) {
    return (((uint64_t)page)>>0) & 0777;
}

physical_address translate_address(virtual_address _virt) {
    uint64_t offset = ((uint64_t)_virt) % PAGE_SIZE;
    page_t virt = containing_address((uint64_t)_virt);

    page_table_t p3 = sub_table_address(((page_table_t)PAGE_TABLE4), p4_index(virt));
    kprintf("p4_index = %47x\n", p4_index(virt));
	kprintf("p3 = %47x\n", p3);
    if (!p3) return 0;

    page_table_t p2 = sub_table_address(p3, p3_index(virt));
	kprintf("p3_index = %47x\n", p3_index(virt));
	kprintf("p2 = %47x\n", p2);
    if (!p2) return 0;
    
    page_table_t p1 = sub_table_address(p2, p2_index(virt));
	if (p1 == -1) {
        page_entry_t p3_entry = p3[p3_index(virt)];
        if (p3_entry.present) {
            void *start_frame = ((uint64_t)(((page_entry_t *)p3) + p3_index(virt))) & 0x000ffffffffff000;
            if (p3_entry.huge_page) {
                ERROR("of poppycock");
            }
        }

		WARN("uh oh you hit a big page!");
    }
	kprintf("p2_index = %47x\n", p2_index(virt));
	kprintf("p1 = %47x\n", p1);
    if (!p1) return 0;
    
    page_entry_t _frame = p1[p1_index(virt)];
    if (!_frame.present)return 0;

    uint64_t __frame = _frame.packed;
    frame_t frame = starting_address(containing_address(__frame & 0x000ffffffffff000)) | offset;
}

void set_addr_mask(page_entry_t *entry, uint64_t addr) {
    uint64_t mask = 0xfff0000000000fff;
    entry->packed &= mask;
    entry->packed |= (addr * PAGE_SIZE);
}

page_table_t create_if_nonexistant(page_table_t in, uint64_t index, frame_allocator *alloc) {
    page_entry_t location = in[index];
    page_table_t res;
    if (!location.present) {
        if (location.huge_page) {
            ERROR("huge pages not allowed!");
        }
        frame_t frame = allocate_frame(alloc);
        if (!frame) {
            ERROR("no frames availible");
        }
        set_addr_mask(&in[index], frame);
        in[index].present = 1;
        in[index].writable = 1;
        res = sub_table_address(in, index);
        memset(res, 0, sizeof(page_entry_t) * PAGE_ENTRIES);
    }
    res = sub_table_address(in, index);
    return res;
}

void identity_map(frame_t frame, uint8_t flags, frame_allocator *alloc) {
    map_page_to_frame(
            containing_address(starting_address(frame)),
            frame,
            flags,
            alloc);
}

void unmap_page(page_t page, frame_allocator *alloc) {
    page_table_t p4 = (page_table_t)PAGE_TABLE4;
    if (!p4[p4_index(page)].present) {
        WARN("cannot unmap frame as it is not present");
        return;
    }
    page_table_t p3 = sub_table_address(p4, p4_index(page));

    if (!p3[p3_index(page)].present) {
        WARN("cannot unmap frame as it is not present");
        return;
    }
    page_table_t p2 = sub_table_address(p3, p3_index(page));

    if (!p2[p2_index(page)].present) {
        WARN("cannot unmap frame as it is not present");
        return;
    }
    page_table_t p1 = sub_table_address(p2, p2_index(page));

    if (!p1[p1_index(page)].present) {
        WARN("cannot unmap frame as it is not present");
        return;
    }
    p1[p1_index(page)].present = 0;
    WARN("FRAME NOT BEING FREED. GET BETTER FRAME ALLOCATOR");
}


void map_page(page_t page, uint8_t flags, frame_allocator *alloc) {
    frame_t frame = allocate_frame(alloc);
    if (!frame) {
        ERROR("no frames availible");
    }
    map_page_to_frame(page, frame, flags, alloc);
}

void map_page_to_frame(page_t page, frame_t frame, uint8_t flags, frame_allocator *alloc) {
    page_table_t p4 = (page_table_t)PAGE_TABLE4;
    page_table_t p3 = create_if_nonexistant(p4, p4_index(page), alloc);
    page_table_t p2 = create_if_nonexistant(p3, p3_index(page), alloc);
    page_table_t p1 = create_if_nonexistant(p2, p2_index(page), alloc);

    if (p1[p1_index(page)].present) {
        ERROR("cannot remap a present page to new frame!");
    }
    set_addr_mask(&p1[p1_index(page)], frame);
    p1[p1_index(page)].present = 1;
}


void discover_next_free_frame(frame_allocator *alloc) {
    struct memory_area *area = 0;
    
    void filter_pick(struct memory_area *m) {
        uint64_t addr = m->base_addr + m->length - 1;
        if (containing_address(addr) >= alloc->current_frame_index) {
            if (area==0 || area->base_addr > m->base_addr) {
                area = m;
            }
        }
    }
    alloc->area = area;
    itr_memory_mapped_locations(alloc->mem_info, filter_pick);
    if (area) {
        uint64_t start_frame = containing_address((uint64_t)area);
        if (alloc->current_frame_index < start_frame) {
            alloc->current_frame_index = start_frame;
        }
    }
}

uint64_t allocate_frame(frame_allocator *alloc) {
    if (alloc->current_frame_index == 0) {
        return 0;
    }
    uint64_t frame;
repeat:
    frame = alloc->current_frame_index;
    uint64_t current_area_last_frame = containing_address(
            (uint64_t)(alloc->area->base_addr + alloc->area->length - 1));
    if (frame > current_area_last_frame) {
        discover_next_free_frame(alloc);
    } else if (frame >= alloc->kernel_start && frame <= alloc->kernel_end) {
        kprintf("discovered kernel_frame @#%05x\n", frame);
        alloc->current_frame_index = alloc->kernel_end + 1;
    } else if (frame >= alloc->mboot_start && frame <= alloc->mboot_end) {
        kprintf("discovered multiboot_frame @#%05x\n", frame);
        alloc->current_frame_index = alloc->mboot_end + 1;
    } else {
        alloc->current_frame_index++;
        return frame;
    }
    goto repeat;
}


frame_allocator init_allocator(struct memory_map_tag *info, physical_address ks,
                                                              physical_address ke,
                                                              physical_address ms,
                                                                physical_address me) {
    frame_allocator res = {
        .current_frame_index = 0,
        .area = 0,
        .mem_info = info,
        .kernel_start = containing_address((uint64_t)ks),
        .kernel_end = containing_address((uint64_t)ke),
        .mboot_start = containing_address((uint64_t)ms),
        .mboot_end = containing_address((uint64_t)me),
    };

    discover_next_free_frame(&res);
    return res;
}
