
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
        *huge = 0;
        return referenced_table(table[at_index]);
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

uint64_t get_page_index(uint8_t pagenum, virtual_address va) {
    page_t VA = containing_address(VA);
    page_table_t p4 = (page_table_t)PAGE_TABLE4;
    if (pagenum == 4) {
        return (uint64_t)p4;
    }
    page_table_t p3 = referenced_table(p4[p4_index(VA)]);
    if (pagenum == 3) {
        return (uint64_t)p3;
    }
    page_table_t p2 = referenced_table(p3[p3_index(VA)]);
    if (pagenum == 2) {
        return (uint64_t)p2;
    }
    page_table_t p1 = referenced_table(p2[p2_index(VA)]);
    if (pagenum == 1) {
        return (uint64_t)p1;
    }
    return 0;
}

page_table_t show_page_entry(page_table_t table, uint64_t at_index) {
    page_entry_t pet = table[at_index];
    uint64_t table_address = 0x0;
    if (pet.present) {
        table_address = (uint64_t)referenced_table(pet);
        kprintf(
                table_address ?
                "       %03x %03x %03b %03b  %03b   %03b  %03b %03b %03b %03b %03b\n":
                "       %03x %03x      %03b %03b  %03b   %03b  %03b %03b %03b %03b %03b\n",
                table,
                table_address,
                pet.writable,
                pet.user_accessable,
                pet.write_thru_cache,
                pet.disable_cache,
                pet.accessed,
                pet.dirty,
                pet.huge_page,
                pet.global,
                pet.no_execute);
        if (pet.huge_page) {
            table_address = 0x0;
        }
    }
    return (page_table_t)table_address;
}

void show_page_table_layout_for_address(uint64_t address) {
    page_t page = containing_address(address);
    kprintf("  %03x is in page #%03x\n", address, page);
    uint64_t offs[4] = {
        p1_index(page),
        p2_index(page),
        p3_index(page),
        p4_index(page)
    };
    kprintf("  The offsets in tables for this address are:\n");
    kprintf("    p4: %03x\n"
            "    p3: %03x\n"
            "    p2: %03x\n"
            "    p1: %03x\n", offs[3], offs[2], offs[1], offs[0]);
    kprintf("entry cascade: table    next     W U WTC DiC A D H G NeX\n");
    int _page = 4;
    page_table_t next = (void *)translate_address((void *)PAGE_TABLE4);
    while(next && _page) {
        _page--;
        kprintf("  (%03i[%07i])", _page+1, offs[_page]);
        next = show_page_entry(next, offs[_page]);
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
frame_t translate_page(page_t virt) {
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
        frame_t frame = 0;
        if (alloc == NULL) {
            frame = allocate_frame();
        } else {
            frame = early_allocate_frame(alloc);
        }
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

void unmap_page(page_t page) {
    frame_t frame = translate_page(page);
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
    free_frame(frame);
}


frame_t map_page(page_t page, uint8_t flags, frame_allocator *alloc) {
    frame_t frame = early_allocate_frame(alloc);
    if (!frame) {
        ERROR("no frames availible");
    }
    map_page_to_frame(page, frame, flags, alloc);
    return frame;
}

void map_page_to_frame(page_t page, frame_t frame, uint8_t flags, frame_allocator *alloc) {
    if (translate_page(page) == frame) {
        return; // no mapping required!
    }
    //TODO set flags
    page_table_t p4 = (page_table_t)PAGE_TABLE4;
    page_table_t p3 = create_if_nonexistant(p4, p4_index(page), alloc);
    page_table_t p2 = create_if_nonexistant(p3, p3_index(page), alloc);
    page_table_t p1 = create_if_nonexistant(p2, p2_index(page), alloc);
    if (p1) { // p2 is a normal page table, procede normally
        if (p1[p1_index(page)].present) {
            WARN("mapping a present page to new frame!");
        }
        set_addr_mask(&p1[p1_index(page)], frame);
        p1[p1_index(page)].present = 1;
        p1[p1_index(page)].writable = 1;
    } else { // p2 is a huge page table, so some special stuff
        page_entry_t _frame = p2[p2_index(page)];
        if (_frame.present) {
            WARN("mapping a present page to new frame!");
        }
        set_addr_mask(&p2[p2_index(page)], frame&0xfffffffffffffe00);
        p2[p2_index(page)].present = 1;
        p2[p2_index(page)].writable = 1;
    }

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

uint64_t early_allocate_frame(frame_allocator *alloc) {
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
        alloc->current_frame_index = alloc->kernel_end + 1;
    } else if (frame >= alloc->mboot_start && frame <= alloc->mboot_end) {
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

frame_list_t *split_block_at(frame_list_t *, frame_t);

void assign_address(page_t page) {
    frame_t f = allocate_frame();
    map_page_to_frame(page, f, 0, NULL);
}

frame_list_t *list_head = NULL;
frame_list_t *list_next = NULL;



frame_t allocate_frame() {
    frame_list_t *ll = list_head;
    while(ll && LIST_STATUS(ll->next)) {
        ll = LIST_POINTER(ll->next);
    }
    if (ll) {
        frame_t allocd = ll->start;
        split_block_at(ll, ll->start);
        return allocd;
    }
    WARN("OUT OF MEMORY");
    return 0;
}

void free_frame(frame_t t) {
    frame_list_t *ll = list_head;
    while(ll) {
        if (ll->start <= t && ll->end >= t) {
            if (LIST_STATUS(ll->next)) {
                split_block_at(ll, t);
            }
        }
        ll = LIST_POINTER(ll->next);
    }
}

frame_list_t *frame_list_alloc() {
    // we will be returning list_next, so zero it
    memset(list_next, 0, sizeof(frame_list_t));

    // set the next address
    list_next++;

    // we can only fit 16 list structures per page/frame
    // when we hit 14, allocate a new structure. it has to be 14
    // because there is the chance that allocating a new structure
    // will also require a list allocation, and that could cause an inf loop
    uint64_t nextframeaddr = (uint64_t)(list_next+1);
    if (nextframeaddr % PAGE_SIZE == 0) {
        page_t nextpage = containing_address(nextframeaddr);
        frame_t is_frame = translate_page(nextpage);

        // this page is not mapped
        if (!is_frame) {
            is_frame = allocate_frame();
            map_page_to_frame(nextpage, is_frame, 0, NULL);
        }
        // TODO mark page allocated
    }
    return list_next-1;
}

void free_LIST(frame_list_t *fat) {
    list_next--;
    if (fat == list_next) {
        return;
    }

    // we're going to shift the most recently used into the freed memory space!
    frame_list_t *last_allocated = list_next;

    // copy memory into freed chunk
    memcpy(fat, last_allocated, sizeof(frame_list_t));

    // get the previous and next entries
    // since fat can never be the only entry, fprev MUST be valid. fnext need not be
    frame_list_t *fprev = LIST_POINTER(fat->prev);
    uint64_t fpstat = LIST_STATUS(fprev->next);
    fprev->next = (void *)(((uint64_t)fat) | fpstat);

    frame_list_t *fnext = LIST_POINTER(fat->next);
    if (fnext) { // could potentially be a null pointer!
        uint64_t fnstat = LIST_STATUS(fnext->prev);
        fnext->prev = (void *)(((uint64_t)fat) | fnstat);
    }
}

void set_list_head(frame_list_t *l) {
    list_head = l;
}

void merge_LIST(frame_list_t *A, frame_list_t *B) {
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
        A->next = (void *)(((uint64_t)B)|LIST_STATUS(A->next));
        return;
    }
    A->end = B->end;
    frame_list_t *next = LIST_POINTER(B->next);
    if (next) {
        // there is another block after B, which needs to point back to A now
        next->prev = (void *)(((uint64_t)A)|LIST_STATUS(next->prev));
    }
    A->next = B->next;
    free_LIST(B);
}

frame_list_t *split_block_at(frame_list_t *fat, frame_t frame) {
    if (fat->start == fat->end) {
        // there was only one element, this LIST should be destroyed!
        frame_list_t *prev = LIST_POINTER(fat->prev);
        frame_list_t *next = LIST_POINTER(fat->next);
        free_LIST(fat);
        merge_LIST(prev, next);
        return NULL;
    }
    if (frame == fat->start) {
        // losing from beginning
        fat->start++;
        frame_list_t *prev = LIST_POINTER(fat->prev);
        if (prev) { // this is a block that is NOT the first block...
            prev->end++;
            return prev;
        } else {
            // allocate new block at beginning
            frame_list_t *newblck = frame_list_alloc();
            newblck->start = frame;
            newblck->end = frame;
            uint64_t flipstatus = LIST_STATUS(fat->prev)?0:1;
            newblck->prev = (void *)flipstatus;
            newblck->next = (void *)(((uint64_t)fat) | flipstatus);
            fat->prev = (void *)(((uint64_t)newblck)|LIST_STATUS(fat->prev));
            set_list_head(newblck);
            return newblck;
        }
    } else if (frame == fat->end) {
        // losing from the end
        fat->end--;
        frame_list_t *next = LIST_POINTER(fat->next);
        if (next) { // we're not the last block!
            next->start--;
        } else {
            frame_list_t *newblck = frame_list_alloc();
            newblck->start = frame;
            newblck->end = frame;
            uint64_t flipstatus = LIST_STATUS(fat->prev)?0:1;
            newblck->prev = (void *)(((uint64_t)fat) | flipstatus);
            newblck->next = (void *)flipstatus;
            fat->next = (void *)(((uint64_t)newblck)|LIST_STATUS(fat->next));
        }
        return fat;
    } else {
        // its in the middle of our block :(
        frame_list_t *newblck = frame_list_alloc();
        frame_list_t *second_half = frame_list_alloc();

        newblck->start = newblck->end = frame;
        uint64_t flipstatus = LIST_STATUS(fat->prev)?0:1;
        newblck->prev = (void *)(((uint64_t)fat)|flipstatus);
        newblck->next = (void *)(((uint64_t)second_half)|flipstatus);

        second_half->start = frame+1;
        second_half->end = fat->end;
        second_half->prev = (void *)(((uint64_t)newblck)|LIST_STATUS(fat->next));
        second_half->next = fat->next;

        frame_list_t *nnn = LIST_POINTER(fat->next);
        if (nnn) {
            nnn->prev = (void *)(((uint64_t)second_half)|LIST_STATUS(nnn->prev));
        }

        fat->next = (void *)(((uint64_t)newblck)|LIST_STATUS(fat->next));
        fat->end = frame-1;
        return newblck;
    }
}

void _mark_frame_allocated(frame_t frame) {
    frame_list_t *fat = list_head;
loop:
    if (fat == 0) {
        return;
    }
    if (!(LIST_CHECKSUM(fat->next, fat->prev))) {
        WARN("corrupt block");
        return; // corrupt block anyway
    }

    if (frame >= fat->start && frame <= fat->end) {
        // block is already allocated
        if(LIST_STATUS(fat->next)) {
            // do nothing
        } else {
            split_block_at(fat, frame);
        }
        return;
    }
    fat = LIST_POINTER(fat->next);
    goto loop;
}

void _mark_contig_allocated(frame_t start, frame_t end) {
    frame_list_t *fat = list_head;
loop:
    if (fat == 0) {
        return;
    }
    if (!(LIST_CHECKSUM(fat->next, fat->prev))) {
        WARN("corrupt block");
        return; // corrupt block anyway
    }

    if (start >= fat->start && start <= fat->end) {
        if (end > fat->end) {
            WARN("Can't create contig alloc across blocks yet");
            return;
        }
        // block is already allocated
        if(LIST_STATUS(fat->next)) {
            // do nothing
        } else {
            fat = split_block_at(fat, start);
            fat->end = end;
            fat = LIST_POINTER(fat->next);
            if (fat) {
                fat->start = end+1;
            }
        }
        return;
    }
    fat = LIST_POINTER(fat->next);
    goto loop;
}

void print_frame_alloc_table_list_entry(uint64_t entry) {
    frame_list_t *st = list_head;
    for(int i=0;i<entry;i++) {
        if (!st) {
            kprintf("there are only %03i LIST list entries\n", i);
            return;
        }
        st = LIST_POINTER(st->next);
    }
    if (!st) {
        kprintf("there are only %03i LIST list entries\n", entry);
        return;
    }
    kprintf("LIST list entry #%03i (%03x):\n", entry, st);
    kprintf("  starting frame = %03x\n", st->start);
    kprintf("  ending frame   = %03x\n", st->end);
    kprintf("  alloc status   = %03s\n", LIST_STATUS(st->next)?"allocated":"free");
    kprintf("  next entry     = %03x\n", LIST_POINTER(st->next));
    kprintf("  prev entry     = %03x\n", LIST_POINTER(st->prev));
}

frame_list_t *vpage_allocator(frame_allocator *fa, frame_t p1allocd) {
    frame_t LIST_frame = early_allocate_frame(fa);
    page_t alloc_housing = containing_address(0x500000000);
    list_next = (void *)starting_address(alloc_housing);
    map_page_to_frame(alloc_housing, LIST_frame, 0, fa);
    list_head = frame_list_alloc();
    bounds_t membounds = get_large_ram_area(fa->mem_info);
    list_head->start = containing_address(membounds.start);
    list_head->end = containing_address(membounds.end);
    list_head->next = (void *)(0);
    list_head->prev = (void *)(0);

    _mark_contig_allocated((fa->kernel_start), (fa->kernel_end));
    _mark_contig_allocated((fa->mboot_start), (fa->mboot_end));
    _mark_contig_allocated(p1allocd, fa->current_frame_index - 1);

    return (frame_list_t *)0;
}

frame_t map_out_huge_pages(frame_allocator *fa) {
    frame_t nf = early_allocate_frame(fa);
    identity_map(nf, 0, NULL);
    page_entry_t *np1 = (void *)starting_address(nf);
    memset(np1, 0, sizeof(page_entry_t)*512);
    for(int i=0;i<512;i++) {
        np1[i].present = 1;
        np1[i].writable = 1;
        np1[i].user_accessable = 0;
        np1[i].write_thru_cache = 1;
        np1[i].disable_cache = 1;
        np1[i].accessed = 1;
        np1[i].dirty = 0;
        np1[i].huge_page = 0;
        np1[i].global = 0;
        np1[i].no_execute = 0;
        np1[i]._addr_mask = i;
    }
    page_entry_t nent;
    nent.packed = 0;
    nent.present = 1;
    nent.writable = 1;
    nent.user_accessable = 0;
    nent.write_thru_cache = 0;
    nent.disable_cache = 0;
    nent.accessed = 1;
    nent.dirty = 0;
    nent.huge_page = 0;
    nent.global = 0;
    nent.no_execute = 0;
    nent._addr_mask = containing_address((uint64_t)np1);
    page_table_t fp2 = (void *)get_page_index(2, (void *)0x100000);
    fp2[0].packed = nent.packed;
    for(int i=1;i<PAGE_ENTRIES;i++) {
        fp2[i].packed = 0;
    }
    return nf;
}
