#include "ktype.h"
#include "libk.h"

#include "kmalloc.h"
#include "paging.h"

#ifdef kmalloc_best_fit
avl_stack_t *NEXT_FREE_AVL = NULL;
avl_t *AVL_HEAD = NULL;
#endif

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

#ifdef kmalloc_best_fit
void avl_free(avl_t *avl) {
    //free(avl);
}

avl_t *avl_malloc() {
    //avl_t *avl = malloc(sizeof(avl_t) * 1);
    //return avl;
    return NULL;
}
#endif


void mem_init() {
    uint64_t kernel_heap_start = 0x7ffffffff8;
    uint64_t kernel_heap_end =   0x8ffffff000;
    uint64_t kernel_heap_size =  0x0ffffff000;

    MEM_HEAD = (char *)kernel_heap_start;
 

#ifdef kmalloc_best_fit
    uint64_t initial_avl_stack = size_round(sizeof(avl_t) * 5);
    alloc_hdr_t *list = (alloc_hdr_t *)MEM_HEAD;
    alloc_hdr_t *lend = list + (initial_avl_stack / sizeof(alloc_hdr_t)) - 1;
    force_paging(list);
    force_paging(lend);
    list->packed = initial_avl_stack | 0x1;
    lend->packed = initial_avl_stack | 0x1;
    
    uint64_t free_avl_stack = kernel_heap_size - initial_avl_stack;
    list = lend+1;
    lend = list + (free_avl_stack / sizeof(alloc_hdr_t)) - 1;
    force_paging(list);
    force_paging(lend);
    lend->packed = free_avl_stack;
    list->packed = free_avl_stack;

    MEM_END = (char *)lend;
#else
    alloc_hdr_t *list = (alloc_hdr_t *)MEM_HEAD;
    alloc_hdr_t *lend = list + (kernel_heap_size / sizeof(alloc_hdr_t)) - 1;
    force_paging(list);
    force_paging(lend);
    lend->packed = kernel_heap_size;
    list->packed = kernel_heap_size;
    MEM_END = (char *)lend;
#endif

    

 

    /* this goes in the ifdef!
    alloc_hdr_t *lend = list+(kernel_heap_size/sizeof(alloc_hdr_t))-1;

    char *mem = MEM_HEAD = malloc(sizeof(char) * 1024);
    alloc_hdr_t *list = (alloc_hdr_t *)mem;
    list->packed = 1024;
    alloc_hdr_t *lend = list-1+(1024/sizeof(alloc_hdr_t));
    lend->packed = 1024;
    MEM_END = (char *)lend;

    AVL_HEAD = avl_malloc();
    AVL_HEAD->smaller = NULL;
    AVL_HEAD->larger = NULL;
    AVL_HEAD->ref = list;
    */
}




uint64_t size_round(uint64_t size) {
    int i = 0x40; // 64
    while(size > i-16) { // 16 bytes are needed for the cap and base
        i<<=1;
    }
    return i;
}

#ifdef kmalloc_best_fit
avl_t *avl_add(avl_t *avl, avl_t *navl) {
    if (avl == NULL) {
        return navl;
    }
    if (navl == NULL) {
        return avl;
    }

    if (block_size(avl->ref) >= block_size(navl->ref)) {
        avl->smaller = avl_add(avl->smaller, navl);
    }
    else if (block_size(avl->ref) < block_size(navl->ref)) {
        avl->larger = avl_add(avl->larger, navl);
    }
    return avl;
}
#endif

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


#ifdef kmalloc_best_fit
    avl_t *navl = avl_malloc();
    navl->smaller = NULL;
    navl->larger = NULL;
    navl->ref = new_start;
    AVL_HEAD = avl_add(AVL_HEAD, navl);
#endif
    
    return block+1;
}


#ifdef kmalloc_best_fit
// this needs to be implemented to be a real avl!
avl_t *rebalance(avl_t *avl) {
    return avl;
}

avl_t *avl_merge(avl_t *A, avl_t *B) {
    if (!A) {
        return B;
    }
    if (!B) {
        return A;
    }
    avl_t *AG = A->larger;
    A->larger = B;
    B->smaller = avl_merge(AG, B->smaller);
    return A;
}

avl_t *avl_search_and_remove(avl_t *parent, uint64_t size, avl_t **write) {
    if (parent == NULL) {
        *write = NULL;
        return NULL;
    }
    if (parent->ref->packed == size) { // has to have free bit anyway
        *write = parent;
        return avl_merge(parent->smaller, parent->larger);
    }
    if (block_size(parent->ref) > size) {
        if (parent->smaller == NULL) {
            *write = parent;
            return parent->larger;
        }
        parent->smaller = avl_search_and_remove(parent->smaller, size, write);
        return rebalance(parent);
    }
    if (block_size(parent->ref) < size) {
        if (parent->larger == NULL) {
            *write = parent;
            return parent->smaller;
        }
        parent->larger = avl_search_and_remove(parent->larger, size, write);
        return rebalance(parent);
    }
    *write = NULL;
    return NULL;
}
#endif

alloc_hdr_t *get_best_fit(uint64_t size) {
    if (size_round(size-16) != size) { // bad size!
        return NULL;
    }

#ifdef kmalloc_best_fit
    avl_t *res = NULL;
    AVL_HEAD = avl_search_and_remove(AVL_HEAD, size, &res);
    if (res == NULL) {
        // we couldn't find a block!
        return NULL;
    }

    if (block_size(res->ref) != size) {
        //TODO splice, add back in!
    }
    alloc_hdr_t *result = res->ref;
    avl_free(res);
    return result;
#else

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
#endif
}

#ifdef kmalloc_best_fit
avl_t *_avl_extract_by_ref(avl_t *parent, alloc_hdr_t *ref, avl_t **write) {
    if (parent == NULL) {
        *write = NULL;
        return NULL;
    }
    if (parent->ref == ref) {
        *write = parent;
        return avl_merge(parent->smaller, parent->larger);
    }

    parent->smaller = _avl_extract_by_ref(parent->smaller, ref, write);
    if (*write == NULL) {
        parent->larger = _avl_extract_by_ref(parent->larger, ref, write);
    }

    return parent;
}

avl_t *avl_extract_by_ref(alloc_hdr_t *ref) {
    avl_t *res = NULL;
    AVL_HEAD = _avl_extract_by_ref(AVL_HEAD, ref, &res);
    return res;
}
#endif

void kfree(void *ptr) {
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
    
    alloc_hdr_t *next_ref = REF_END + 1;
    if (!block_alloc(next_ref)) {
        uint64_t next_size = block_size(next_ref);
        alloc_hdr_t *new_end = next_ref + (next_size/sizeof(alloc_hdr_t)) - 1;
        uint64_t new_size = blocksize + next_size;
        new_end->packed = REF->packed = new_size;
#ifdef kmalloc_best_fit
        avl_t *avl = avl_extract_by_ref(next_ref);
        avl_free(avl);
#endif
    }

    alloc_hdr_t *ref_prev_end = REF-1;
    if (!block_alloc(ref_prev_end)) {
        uint64_t prevblksize = block_size(ref_prev_end);
        alloc_hdr_t *ref_prev_start = ref_prev_end+1-(prevblksize/sizeof(alloc_hdr_t));
       
        blocksize = block_size(REF); // may have changed! recalc
        alloc_hdr_t *ref_end = REF + (blocksize / sizeof(alloc_hdr_t)) - 1;
        uint64_t new_size = blocksize + prevblksize;
        ref_prev_start->packed = ref_end->packed = new_size;
#ifdef kmalloc_best_fit
        avl_t *avl = avl_extract_by_ref(ref_prev_start);
        avl_free(avl);
#endif
        REF = ref_prev_start;
    }


#ifdef kmalloc_best_fit
    avl_t *avl = avl_malloc();
    avl->smaller = avl->larger = NULL;
    avl->ref = REF;
    AVL_HEAD = avl_add(AVL_HEAD, avl);
#endif
}




#ifdef kmalloc_best_fit
void _avl_print(avl_t *avl, int indent) {
    for(int i=0; i<indent; i++) {
        printf(" ");
    }
    if (!avl) {
        printf("NULL\n");
        return;
    }
    printf("size: %i [0x%x]\n", avl->ref->packed, avl->ref);
    _avl_print(avl->larger, indent+1);
    _avl_print(avl->smaller, indent+1);
    return;
}

void avl_print() {
    if (!AVL_HEAD) {
        kprintf("NO OPEN BLOKS\n");
        return;
    }
    _avl_print(AVL_HEAD, 0);
    return;
}
#endif

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

