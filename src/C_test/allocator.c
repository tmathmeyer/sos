

/* ALWAYS KEEP 4 WAITING PAGES */
struct page_waiting {
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

void fill_missing_pages() {
    while(pagebuffer.missing) {
        if (!pagebuffer.A) {
            pagebuffer.A = allocate_full_page();
        }
        if (!pagebuffer.B) {
            pagebuffer.B = allocate_full_page();
        }
        if (!pagebuffer.C) {
            pagebuffer.C = allocate_full_page();
        }
        if (!pagebuffer.D) {
            pagebuffer.D = allocate_full_page();
        }
        pagebuffer.missing -= 1;
    }
}

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
        

uint64_t singe_mapped_page_claim() {
    fill_missing_pages();
    uint64_t page = pagebuffer.page_A;
    pagebuffer.page_A = 0;
    pagebuffer.missing += 1;
    fill_missing_pages();
    return page;
}

page_table_t map_subpage_table(page_table_t parent_page, int table_index) {
    page_entry_t index = parent_page[table_index];
    if (index.present) {
        return referenced_table(index);
    }
    uint64_t page = grab_next_page_quiet();
    memset(starting_address(page), 0, sizeof(page_table_t));

    set_addr_mask(&index, grab_next_page_quiet());
    index.present = 1;
    index.writable = 1;
    return referenced_table(index);
}

void map_page_to_frame(uint64_t vpage, uint64_t pframe) {
    if (translate_page(vpage) == pframe) {
        return; // no mapping required!
    }
    
    page_table_t p4 = (page_table_t)PAGE_TABLE4;
    page_table_t p3 = map_subpage_table(p4, p4_index(page));
    page_table_t p2 = map_subpage_table(p3, p3_index(page));
    page_table_t p1 = map_subpage_table(p2, p2_index(page));
    
    if (p1[p1_index(page)].present) {
        WARN("mapping a present page to new frame!");
    }
    set_addr_mask(&p1[p1_index(page)], frame);

    p1[p1_index(page)].present = 1;
    p1[p1_index(page)].writable = 1;
}

uint64_t allocate_full_page() {
    uint64_t page = get_next_free_page();
    uint64_t frame = get_next_free_frame();
    map_page_to_frame(page, frame);
    return page;
}
