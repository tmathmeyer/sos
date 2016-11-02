#include "ktype.h"
#include "libk.h"

#include "kmalloc.h"
#include "paging.h"

char *MEM_HEAD = NULL;
char *MEM_END = NULL;

void force_paging(void *ptr) {
    physical_address phys = translate_address(ptr);
    if (!phys) { // we dont have a physical address for this virt addr
        uint64_t frame = allocate_frame();
        uint64_t page  = containing_address((uint64_t)ptr);
        map_page_to_frame(page, frame, 0, NULL);
    }
}

void *kmalloc(uint64_t size) {
    uint64_t needed_size = size_round(size);
    alloc_hdr_t *block = get_best_fit(needed_size);
    force_paging(block);
    if (block) {
        return allocate_in_block(block, needed_size);
    }
    return NULL;
}

void mem_init() {
    /*
    uint64_t kernel_heap_start = 0x7ffffffff8;
    uint64_t kernel_heap_end =   0x8ffffff000;
    uint64_t kernel_heap_size =  0x0ffffff000;
    */
    uint64_t kernel_heap_start = 0x8000000000;
    uint64_t kernel_heap_end =   0x8ffffff000;
    uint64_t kernel_heap_size =  0x0ffffff000;

    MEM_HEAD = (char *)kernel_heap_start;
 

    alloc_hdr_t *list = (alloc_hdr_t *)MEM_HEAD;
    alloc_hdr_t *lend = list + (kernel_heap_size / sizeof(alloc_hdr_t)) - 1;
    force_paging(list);
    force_paging(lend);
    lend->packed = kernel_heap_size;
    list->packed = kernel_heap_size;
    MEM_END = (char *)lend;
}




uint64_t size_round(uint64_t size) {
    int i = 0x40; // 64
    while(size > i-16) { // 16 bytes are needed for the cap and base
        i<<=1;
    }
    return i;
}


void *allocate_in_block(alloc_hdr_t *block, uint64_t needed_size) {
    force_paging(block);
    if (block_size(block) == needed_size) { // easy! get_best_fit found a perfect match
        block->packed |= 0x1;
        alloc_hdr_t *nblock = block + (block_size(block) / sizeof(alloc_hdr_t)) - 1;
        force_paging(nblock);
        nblock->packed |= 0x1;
        return block+1; // return the address after the block, this is the alloc'd memory
    }

    // get_best_fit found a block that was too big!
    uint64_t old_size = block_size(block);
    alloc_hdr_t *old_end = block + (old_size / sizeof(alloc_hdr_t)) - 1;
    alloc_hdr_t *new_end = block + (needed_size / sizeof(alloc_hdr_t)) - 1;
    alloc_hdr_t *new_start = new_end+1;
    for(void * i=block; i<(void *)new_end; i+=4096) {
        force_paging(i);
    }
    force_paging(new_start);
    force_paging(new_end);
    force_paging(old_end);

    // set the correct size for the first chunk of the oversized block
    set_block_size(block, needed_size);
    set_block_size(new_end, needed_size);
    
    //set the correct size for the rest of the oversized block
    set_block_size(new_start, old_size - needed_size);
    set_block_size(old_end, old_size - needed_size);

    // set the allocation state
    block->packed |= 0x01;
    new_end->packed |= 0x01;
    new_start->packed &= 0xfffffffffffffffe;
    old_end->packed &= 0xfffffffffffffffe;


    return block+1;
}



alloc_hdr_t *get_best_fit(uint64_t size) {
    if (size_round(size-16) != size) { // bad size!
        return NULL;
    }

    alloc_hdr_t *a = (alloc_hdr_t *)MEM_HEAD;
loop: {
          uint64_t bl_size = block_size(a);
          uint8_t alloc_st = block_alloc(a);
          alloc_hdr_t *e = a-1+(size/sizeof(alloc_hdr_t));
          if (!alloc_st && size <= bl_size) {
              return a;
          }
          if ((char *)e != MEM_END) {
              a = e+1;
              goto loop;
          }
      }
}

void kfree(void *ptr) {
    if (!ptr) {
        return;
    }
    page_t START, END;

    alloc_hdr_t *REF = ptr;
    REF--; // step back to metadata
    if (!block_alloc(REF)) {
        int j = 0;
        int i = 4 / j;
        return;
        //TODO kernel panic!
    }
    (REF->packed) &= 0xffffffffffffffe0;
    uint64_t blocksize = block_size(REF);
    alloc_hdr_t *REF_END = REF + (blocksize / sizeof(alloc_hdr_t)) - 1;
    (REF_END->packed) &= 0xffffffffffffffe0;
    START = containing_address((uint64_t)REF);
    END = containing_address((uint64_t)(REF_END));

    /* check merge next block  */
    alloc_hdr_t *next_ref = REF_END + 1;
    if (translate_address(next_ref) && !block_alloc(next_ref)) {
        uint64_t next_size = block_size(next_ref);
        alloc_hdr_t *new_end = next_ref + (next_size/sizeof(alloc_hdr_t)) - 1;
        uint64_t new_size = blocksize + next_size;
        new_end->packed = REF->packed = new_size;
        if (((uint64_t)new_end)&(~4095) == ((uint64_t)next_ref)&(~4096)) {
            // next free chunk has head and tail in same page;
            // this page must be allocated
        } else {
            // we can also unmap the end alloc_hdr_t page!
            kprintf("nblk=free,ends=diff,updt=END\n");
            END = containing_address((uint64_t)next_ref)+1;
        }
    }

    /* check merge prev block  */
    alloc_hdr_t *ref_prev_end = REF-1;
    if (translate_address(ref_prev_end) && !block_alloc(ref_prev_end)) {
        uint64_t prevblksize = block_size(ref_prev_end);
        alloc_hdr_t *ref_prev_start = ref_prev_end+1-(prevblksize/sizeof(alloc_hdr_t));
        blocksize = block_size(REF); // may have changed! recalc
        alloc_hdr_t *ref_end = REF + (blocksize / sizeof(alloc_hdr_t)) - 1;
        uint64_t new_size = blocksize + prevblksize;
        ref_prev_start->packed = ref_end->packed = new_size;
        REF = ref_prev_start;
        if (((uint64_t)REF)&(~4095) == ((uint64_t)ref_prev_end)&(~4096)) {
            // previous free chunk's head and tail are in the same page
            // we cannot unmap the prev chunk head, since page is allocd
        } else {
            kprintf("pblk=free,ends=diff,updt=START\n");
            START = containing_address((uint64_t)ref_prev_end)-1;
        }
    }
    kprintf("freeing space between [%05x, %05x)\n", START, END);
    START += 1;
    while(START < END) {
        kprintf("unmapping: %03x\n", starting_address(START));
        unmap_page(START);
        START++;
    }
}




void print_mem() {
    alloc_hdr_t *a = (alloc_hdr_t *)MEM_HEAD;
    int i = -1;
loop: {
    uint64_t size = block_size(a);
    uint8_t alloc_st = block_alloc(a);
    alloc_hdr_t *e = a-1+(size/sizeof(alloc_hdr_t));
    kprintf("======================\n");
    kprintf("    size = %05x\n", size);
    kprintf("    stat = [%05i][%05i]\n", alloc_st, block_alloc(e));
    kprintf("    addr = %05x\n", a);
    kprintf("    eadr = %05x\n", e);
    if ((char *)e != MEM_END && i--) {
        a = e+1;
        goto loop;
    }
    kprintf("======================\n\n");
      }
}

