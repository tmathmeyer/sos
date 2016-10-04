
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

page_table_t sub_table_address(page_table_t table, uint64_t at_index, uint8_t *huge) {
    if (table[at_index].present && !table[at_index].huge_page) {
        uint64_t table_address = (uint64_t)table;
        table_address <<= 9;
        table_address |= (at_index << 12);
        *huge = 0;
        return (page_table_t)table_address;
    } else if (table[at_index].huge_page) {
        *huge = 1;
	}
    return 0;
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

page_table_t show_page_entry(page_table_t table, uint64_t at_index) {
    page_entry_t pet = table[at_index];
    uint64_t table_address = 0x0;
    if (pet.present && !pet.huge_page) {
        table_address = (uint64_t)table;
        table_address <<= 9;
        table_address |= (at_index << 12);
    }
    kprintf("          %03b %03b  %03b   %03b  %03b %03b %03b %03b %03x %03b\n",
            pet.writable,
            pet.user_accessable,
            pet.write_thru_cache,
            pet.disable_cache,
            pet.accessed,
            pet.dirty,
            pet.huge_page,
            pet.global,
            table_address,
            pet.no_execute);
    return (page_table_t)table_address;
}

void show_page_table_layout_for_address(uint64_t address) {
    page_t page = containing_address(address);
    kprintf("  %03x is in page #%03i\n", address, page);
    uint64_t p4offset = p4_index(page);
    uint64_t p3offset = p3_index(page);
    uint64_t p2offset = p2_index(page);
    uint64_t p1offset = p1_index(page);
    kprintf("  The offsets in tables for this address are:\n");
    kprintf("    p4: %03x\n"
            "    p3: %03x\n"
            "    p2: %03x\n"
            "    p1: %03x\n", p4offset, p3offset, p2offset, p1offset);
    kprintf("entry cascade: W U WTC DiC A D H G address           NeX\n");
    int _page = 4;
    page_table_t next = (page_table_t)PAGE_TABLE4;
    while(next && _page) {
        kprintf("  (%03i)", _page--);
        next = show_page_entry(next, p4offset);
    }
    kprintf("the physical address associated with this virtual address is\n");
    kprintf("%03x\n", translate_address((void *)address));

}

physical_address translate_page(page_t virt);
physical_address translate_address(virtual_address virt) {
    uint64_t offset = ((uint64_t)virt) % PAGE_SIZE;
    frame_t frame = translate_page(containing_address((uint64_t)virt));
    if (frame) {
        return frame*PAGE_SIZE + offset;
    } else {
        return 0;
    }
}

#define DDDB 0
physical_address translate_page(page_t virt) {
    uint8_t huge_page = 0;

    page_table_t p3 = sub_table_address(((page_table_t)PAGE_TABLE4), p4_index(virt), &huge_page);
    if (DDDB) kprintf("p4_index = %47x\n", p4_index(virt));
	if (DDDB) kprintf("p3 = %47x\n", p3);
    if (!p3) return 0;

    page_table_t p2 = sub_table_address(p3, p3_index(virt), &huge_page);
	if (DDDB) kprintf("p3_index = %47x\n", p3_index(virt));
	if (DDDB) kprintf("p2 = %47x\n", p2);
    if (huge_page) {
        ERROR("1GB HUGE PAGE");
        return 0;
    }
    if (!p2) return 0;
    
    page_table_t p1 = sub_table_address(p2, p2_index(virt), &huge_page);
	if (DDDB) kprintf("p2_index = %47x\n", p2_index(virt));
	if (DDDB) kprintf("p1 = %47x\n", p1);
    if (huge_page) {
        page_entry_t _frame = p2[p2_index(virt)];
        uint64_t __frame = _frame.packed;
        frame_t frame = containing_address(__frame & 0x000ffffffffff000);
        if (frame % PAGE_ENTRIES == 0) {
            frame += p1_index(virt);
            return frame;
        } else {
            ERROR("misaligned 2MB page");
        }
    }
    if (!p1) return 0;
    
    page_entry_t _frame = p1[p1_index(virt)];
    if (!_frame.present)return 0;

    uint64_t __frame = _frame.packed;
    frame_t frame = containing_address(__frame & 0x000ffffffffff000);
}

void set_addr_mask(page_entry_t *entry, uint64_t addr) {
    uint64_t mask = 0xfff0000000000fff;
    entry->packed &= mask;
    entry->packed |= (addr * PAGE_SIZE);
}

page_table_t create_if_nonexistant(page_table_t in, uint64_t index, frame_allocator *alloc) {
    page_entry_t location = in[index];
    page_table_t res;
    uint8_t huge_page;
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
        res = sub_table_address(in, index, &huge_page);
        memset(res, 0, sizeof(page_entry_t) * PAGE_ENTRIES);
    }
    res = sub_table_address(in, index, &huge_page);
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
    uint8_t huge_page;
    if (!p4[p4_index(page)].present) {
        WARN("cannot unmap frame as it is not present");
        return;
    }
    page_table_t p3 = sub_table_address(p4, p4_index(page), &huge_page);

    if (!p3[p3_index(page)].present) {
        WARN("cannot unmap frame as it is not present");
        return;
    }
    page_table_t p2 = sub_table_address(p3, p3_index(page), &huge_page);

    if (!p2[p2_index(page)].present) {
        WARN("cannot unmap frame as it is not present");
        return;
    }
    page_table_t p1 = sub_table_address(p2, p2_index(page), &huge_page);

    if (!p1[p1_index(page)].present) {
        WARN("cannot unmap frame as it is not present");
        return;
    }
    p1[p1_index(page)].present = 0;
    WARN("FRAME NOT BEING FREED. GET BETTER FRAME ALLOCATOR");
}


frame_t map_page(page_t page, uint8_t flags, frame_allocator *alloc) {
    frame_t frame = allocate_frame(alloc);
    if (!frame) {
        ERROR("no frames availible");
    }
    map_page_to_frame(page, frame, flags, alloc);
    return frame;
}

void map_page_to_frame(page_t page, frame_t frame, uint8_t flags, frame_allocator *alloc) {
    page_table_t p4 = (page_table_t)PAGE_TABLE4;
    page_table_t p3 = create_if_nonexistant(p4, p4_index(page), alloc);
    page_table_t p2 = create_if_nonexistant(p3, p3_index(page), alloc);
    page_table_t p1 = create_if_nonexistant(p2, p2_index(page), alloc);

    if (p1[p1_index(page)].present) {
        ERROR("cannot remap a present page to new frame!");
    }
    WARN("cant set flags, will set pres & writable.");
    set_addr_mask(&p1[p1_index(page)], frame);
    p1[p1_index(page)].present = 1;
    p1[p1_index(page)].writable = 1;
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

FAT_list_t *FAT_HEAD = (void *)0;
FAT_list_t *FAT_NEXT_ALLOC = (void *)0;

FAT_list_t *FAT_list_alloc() {
    memset(FAT_NEXT_ALLOC, 0, sizeof(FAT_list_t));
    FAT_NEXT_ALLOC++;
    return FAT_NEXT_ALLOC-1;
}

void free_FAT(FAT_list_t *fat) {
    WARN("FREEING NOT IMPLEMENTED");
    return;
}

void set_list_head(FAT_list_t *l) {
    FAT_HEAD = l;
}

void merge_FAT(FAT_list_t *A, FAT_list_t *B) {
    if (!A || !B) {
        return;
    }
    if (!A) {
        // that means that the block around which these two are being merged
        // WAS THE BEGINNING. that means B is now the beginning!
        set_list_head(B);
        return;
    }
    if (!B) {
        A->next = (void *)(((uint64_t)B)|FAT_STATUS(A->next));
        return;
    }
    A->end = B->end;
    FAT_list_t *next = FAT_POINTER(B->next);
    next->prev = (void *)(((uint64_t)A)|FAT_STATUS(next->prev));
    A->next = (void *)(((uint64_t)next)|FAT_STATUS(A->next));
    free_FAT(B);

}

FAT_list_t *split_block_at(FAT_list_t *fat, frame_t frame) {
    if (fat->start == fat->end) {
        // there was only one element, this FAT should be destroyed!
        FAT_list_t *prev = FAT_POINTER(fat->prev);
        FAT_list_t *next = FAT_POINTER(fat->next);
        merge_FAT(prev, next);
        free_FAT(fat);
        return (void *)0;
    }
    if (frame == fat->start) {
        // losing from beginning
        fat->start++;
        FAT_list_t *prev = FAT_POINTER(fat->prev);
        if (prev) { // this is a block that is NOT the first block...
            prev->end++;
            return prev;
        } else {
            // allocate new block at beginning
            FAT_list_t *newblck = FAT_list_alloc();
            newblck->start = frame;
            newblck->end = frame;
            uint64_t flipstatus = FAT_STATUS(fat->prev)?0:1;
            newblck->prev = (void *)flipstatus;
            newblck->next = (void *)(((uint64_t)fat) | flipstatus);
            fat->prev = (void *)(((uint64_t)newblck)|FAT_STATUS(fat->prev));
            set_list_head(newblck);
            return newblck;
        }
    } else if (frame == fat->end) {
        // losing from the end
        fat->end--;
        FAT_list_t *next = FAT_POINTER(fat->next);
        if (next) { // we're not the last block!
            next->start--;
        } else {
            FAT_list_t *newblck = FAT_list_alloc();
            newblck->start = frame;
            newblck->end = frame;
            uint64_t flipstatus = FAT_STATUS(fat->prev)?0:1;
            newblck->prev = (void *)(((uint64_t)fat) | flipstatus);
            newblck->next = (void *)flipstatus;
            fat->next = (void *)(((uint64_t)newblck)|FAT_STATUS(fat->next));
        }
        return fat;
    } else {
        // its in the middle of our block :(

        FAT_list_t *newblck = FAT_list_alloc();
        FAT_list_t *second_half = FAT_list_alloc();

        newblck->start = newblck->end = frame;
        uint64_t flipstatus = FAT_STATUS(fat->prev)?0:1;
        newblck->prev = (void *)(((uint64_t)fat)|flipstatus);
        newblck->next = (void *)(((uint64_t)second_half)|flipstatus);

        second_half->start = frame+1;
        second_half->end = fat->end;
        second_half->prev = (void *)(((uint64_t)newblck)|FAT_STATUS(fat->next));
        second_half->next = fat->next;
        FAT_list_t *nnn = FAT_POINTER(fat->next);
        if (nnn) {
            nnn->prev = (void *)(((uint64_t)second_half)|FAT_STATUS(nnn->prev));
        }
        fat->next = (void *)(((uint64_t)newblck)|FAT_STATUS(fat->next));
        fat->end = frame-1;
        return newblck;
    }
}

void _mark_frame_allocated(frame_t frame) {
    FAT_list_t *fat = FAT_HEAD;
loop:
    if (fat == 0) {
        return;
    }
    if (!(FAT_CHECKSUM(fat->next, fat->prev))) {
        WARN("corrupt block");
        return; // corrupt block anyway
    }

    if (frame >= fat->start && frame <= fat->end) {
        // block is already allocated
        if(FAT_STATUS(fat->next)) {
            // do nothing
        } else {
            split_block_at(fat, frame);
        }
        return;
    }
    fat = FAT_POINTER(fat->next);
    goto loop;
}

void _mark_contig_allocated(frame_t start, frame_t end) {
    FAT_list_t *fat = FAT_HEAD;
loop:
    if (fat == 0) {
        return;
    }
    if (!(FAT_CHECKSUM(fat->next, fat->prev))) {
        WARN("corrupt block");
        return; // corrupt block anyway
    }

    if (start >= fat->start && start <= fat->end) {
        if (end > fat->end) {
            WARN("Can't create contig alloc across blocks yet");
            return;
        }
        // block is already allocated
        if(FAT_STATUS(fat->next)) {
            // do nothing
        } else {
            fat = split_block_at(fat, start);
            fat->end = end;
            fat = FAT_POINTER(fat->next);
            if (fat) {
                fat->start = end+1;
            }
        }
        return;
    }
    fat = FAT_POINTER(fat->next);
    goto loop;
}

void print_frame_alloc_table_list_entry(uint64_t entry) {
    FAT_list_t *st = FAT_HEAD;
    for(int i=0;i<entry;i++) {
        if (!st) {
            kprintf("there are only %03i FAT list entries\n", i);
            return;
        }
        st = FAT_POINTER(st->next);
    }
    if (!st) {
        kprintf("there are only %03i FAT list entries\n", entry);
        return;
    }
    kprintf("FAT list entry #%03i (%03x):\n", entry, st);
    kprintf("  starting frame = %03x\n", st->start);
    kprintf("  ending frame   = %03x\n", st->end);
    kprintf("  alloc status   = %03s\n", FAT_STATUS(st->next)?"allocated":"free");
    kprintf("  next entry     = %03x\n", FAT_POINTER(st->next));
    kprintf("  prev entry     = %03x\n", FAT_POINTER(st->prev));
}

FAT_list_t *vpage_allocator(frame_allocator *fa) {
    frame_t FAT_frame = allocate_frame(fa);
    page_t alloc_housing = containing_address(0x500000000);
    FAT_NEXT_ALLOC = (void *)starting_address(alloc_housing);
    // TODO: set the identity mapped page for this address to be READ ONLY!!!! (prevent stack fuckery)
    kprintf("frame allocator gave frame with address: %74x\n", starting_address(FAT_frame));
    map_page_to_frame(alloc_housing, FAT_frame, 0, fa);
    FAT_HEAD = FAT_list_alloc();
    FAT_HEAD->start = 0;
    FAT_HEAD->end = 0xffffffffffffffff; // TODO actually get max ram
    FAT_HEAD->next = (void *)(0x00);
    FAT_HEAD->prev = (void *)(0x00);

    _mark_contig_allocated(containing_address(fa->kernel_start), containing_address(fa->kernel_end));
    _mark_contig_allocated(containing_address(fa->mboot_start), containing_address(fa->mboot_end));
 
    //_mark_frame_allocated(FAT_HEAD, fr);
    return (FAT_list_t *)0;
}
