#include "chunk_allocator.h"
#include "ktype.h"
#include "mmu.h"
#include "libk.h"

/*
 * Static variables
 */
static uint64_t (*_get_next_free_frame) (void) = NULL;
static uint64_t (*_get_next_free_page) (void) = NULL;
static void (*_release_frame)(uint64_t) = NULL;
static void (*_release_page)(uint64_t) = NULL;
static struct segment_list *frame_head = NULL;
static struct segment_list *page_head = NULL;
static frame_allocator *level1_allocator = NULL;

/* keep a buffer of waiting mapped addresses */
// TODO: replace with an array...
static struct page_waiting {
    uint64_t page_A;
    uint64_t page_B;
    uint64_t page_C;
    uint64_t page_D;
    uint64_t missing;
} pagebuffer = {
    .page_A = 0,
    .page_B = 0,
    .page_C = 0,
    .page_D = 0,
    .missing = 4
};

/* print the alloc/free frame list */
void print_frames() {
    print_segment_list(frame_head);
}

/* print the alloc/free page list */
void print_pages() {
    print_segment_list(page_head);
}

/* grab a new free frame! */
uint64_t get_next_free_frame() {
    if (_get_next_free_frame) {
        return _get_next_free_frame();
    }
    return 0;
}

/* grab a new free page! */
uint64_t get_next_free_page() {
    if (_get_next_free_page) {
        return _get_next_free_page();
    }
    return 0;
}

/* release a frame */
void release_frame(uint64_t frame) {
    if (_release_frame) {
        return _release_frame(frame);
    }
}

/* release a page */
void release_page(uint64_t page) {
    if (_release_page) {
        return _release_page(page);
    }
}

uint64_t early_allocate_frame(frame_allocator *);
uint64_t _early_allocate_frame() {
    return early_allocate_frame(level1_allocator);
}

void __release_frame(uint64_t segment) {
    release_segment(segment, frame_head);
}

void __release_page(uint64_t segment) {
    release_segment(segment, page_head);
}

uint64_t __get_next_free_frame() {
    return claim_next_free_segment(frame_head);
}

uint64_t __get_next_free_page() {
    return claim_next_free_segment(page_head);
}

void level2_memory_allocator(frame_allocator *alloc, frame_t f) {
    frame_t A = _get_next_free_frame();
    map_page_to_frame(A, A);
    pagebuffer.page_A = A;
    frame_t B = _get_next_free_frame();
    map_page_to_frame(B, B);
    pagebuffer.page_B = B;
    frame_t C = _get_next_free_frame();
    map_page_to_frame(C, C);
    pagebuffer.page_C = C;
    frame_t D = _get_next_free_frame();
    map_page_to_frame(D, D);
    pagebuffer.page_D = D;
    
    frame_t p_frame = _get_next_free_frame();
    map_page_to_frame(p_frame, p_frame);
    // range of "heap" memory
    page_head = create_head((void *)starting_address(p_frame), 0x8000000, 0x8ffffff); 
    
    
    frame_t f_frame = _get_next_free_frame();
    map_page_to_frame(f_frame, f_frame);
    // support 0x10000 frames starting from last used frame
    frame_head = create_head((void *)starting_address(f_frame), f_frame+1, f_frame+0x10000);


    _get_next_free_frame = __get_next_free_frame;
    _get_next_free_page = __get_next_free_page;
    _release_frame = __release_frame;
    _release_page = __release_page;
}

void level1_memory_allocator(frame_allocator *alloc) {
    level1_allocator = alloc;
    _get_next_free_frame = _early_allocate_frame;
    _get_next_free_page = _early_allocate_frame;
}

/* index lookups for pages */
#define P4_INDEX 27
#define P3_INDEX 18
#define P2_INDEX 9
#define P1_INDEX 0
#define p1_index(page) table_index((page), P1_INDEX)
#define p2_index(page) table_index((page), P2_INDEX)
#define p3_index(page) table_index((page), P3_INDEX)
#define p4_index(page) table_index((page), P4_INDEX)
uint64_t table_index(page_t page, uint64_t table_level) {
    return (((uint64_t)page)>>table_level) & 0777;
}

/* convert address -> page */
uint64_t containing_address(uint64_t addr) {
    return addr / PAGE_SIZE;
}

/* convert page -> address */
uint64_t starting_address(uint64_t page_frame) {
    return page_frame * PAGE_SIZE;
}

/* get the next level table from the entry, or NULL */
page_table_t referenced_table(page_entry_t entry) {
    if (entry.present) {
        uint64_t _entry = entry.packed;
        _entry &= 0x000ffffffffff000;
        return (page_table_t)_entry;
    }
    return 0;
}

/* set the next table in this entry */
void set_addr_mask(page_entry_t *entry, uint64_t addr) {
    uint64_t mask = 0xfff0000000000fff;
    entry->packed &= mask;
    entry->packed |= (addr * PAGE_SIZE);
}

/* fill in the page buffer */
void fill_missing_pages() {
    while(pagebuffer.missing) {
        if (!pagebuffer.page_A) {
            pagebuffer.page_A = allocate_full_page();
        }
        if (!pagebuffer.page_B) {
            pagebuffer.page_B = allocate_full_page();
        }
        if (!pagebuffer.page_C) {
            pagebuffer.page_C = allocate_full_page();
        }
        if (!pagebuffer.page_D) {
            pagebuffer.page_D = allocate_full_page();
        }
        pagebuffer.missing -= 1;
    }
}

/* take a page from the page buffer without replacement */
uint64_t grab_next_page_quiet() {
    uint64_t page = 0;
    if (pagebuffer.page_A) {
        page = pagebuffer.page_A;
        pagebuffer.page_A = 0;
    }
    if (pagebuffer.page_B) {
        page = pagebuffer.page_B;
        pagebuffer.page_B = 0;
    }
    if (pagebuffer.page_C) {
        page = pagebuffer.page_C;
        pagebuffer.page_C = 0;
    }
    if (pagebuffer.page_D) {
        page = pagebuffer.page_D;
        pagebuffer.page_D = 0;
    }
    pagebuffer.missing += 1;
    return page;
}

/* take a page and then fill in the buffer again */
uint64_t singe_mapped_page_claim() {
    uint64_t page = grab_next_page_quiet();
    fill_missing_pages();
    return page;
}

/* get the next level page table; create if it doesn't exist */
page_table_t map_subpage_table(page_table_t parent_page, int index_no) {
    page_entry_t index = parent_page[index_no];
    if (!index.present) {
        uint64_t page = grab_next_page_quiet();
        memset((void *)starting_address(page), 0, sizeof(page_table_t));

        set_addr_mask(&index, page);
        index.present = 1;
        index.writable = 1;
    }
    return referenced_table(index);
}

/* link virtual and physical pages in mmu */
void map_page_to_frame(uint64_t vpage, uint64_t pframe) {
    if (translate_page(vpage) == pframe) {
        return; // no mapping required!
    }
    fill_missing_pages();
    
    page_table_t p4 = (page_table_t)PAGE_TABLE4;
    page_table_t p3 = map_subpage_table(p4, table_index(vpage, P4_INDEX));
    page_table_t p2 = map_subpage_table(p3, table_index(vpage, P3_INDEX));
    page_table_t p1 = map_subpage_table(p2, table_index(vpage, P2_INDEX));
    
    uint64_t p1_index = table_index(vpage, P1_INDEX);
    if (p1[p1_index].present) {
        WARN("mapping a present page to new frame!");
    }
    set_addr_mask(&p1[p1_index], pframe);

    p1[p1_index].present = 1;
    p1[p1_index].writable = 1;
}

/* grab a new page, already mmu mapped */
uint64_t allocate_full_page() {
    uint64_t page = get_next_free_page();
    uint64_t frame = get_next_free_frame();
    kprintf("AFP: page=%6ex frame=%6ex\n", page, frame);
    map_page_to_frame(page, frame);
    kprintf("AFP: missing = %6ex\n", pagebuffer.missing);
    fill_missing_pages();
    return page;
}

/* translate a virtual address to a physical one */
physical_address translate_address(virtual_address virt) {
    uint64_t offset = ((uint64_t)virt) % PAGE_SIZE;
    frame_t frame = translate_page(containing_address((uint64_t)virt));
    if (frame) {
        return frame*PAGE_SIZE + offset;
    } else {
        return 0;
    }
}

frame_t translate_page(page_t virt) {
    uint8_t huge_page = 0;

    page_table_t p3 = sub_table_address(((page_table_t)PAGE_TABLE4), p4_index(virt), &huge_page);
    if (!p3) return 0;

    page_table_t p2 = sub_table_address(p3, p3_index(virt), &huge_page);
    if (huge_page) {
        ERROR("1GB HUGE PAGE");
        return 0;
    }
    if (!p2) return 0;

    page_table_t p1 = sub_table_address(p2, p2_index(virt), &huge_page);
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
    return frame;
}

/* unmap the virtual address from its physical address. free physical address */
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
    release_frame(frame);
}










/* very early frame operations */
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

/* very early frame operations */
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

/* very early frame operations */
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



















/* get the next level page table; writeback true for a huge page */
page_table_t sub_table_address(page_table_t table, uint64_t at_index, uint8_t *huge) {
    if (table[at_index].present && !table[at_index].huge_page) {
        *huge = 0;
        return referenced_table(table[at_index]);
    } else if (table[at_index].huge_page) {
        *huge = 1;
    }
    return 0;
}

/* support for mapping out huge pages */
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

/* we don't like huge pages 'round these parts */
frame_t map_out_huge_pages(frame_allocator *fa) {
    frame_t nf = early_allocate_frame(fa);
    map_page_to_frame(containing_address(starting_address(nf)), nf);
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
