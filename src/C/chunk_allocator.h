#ifndef chunk_allocator_h
#define chunk_allocator_h

#include "ktype.h"

struct segment_list {
    uint8_t allocated : 1;
    uint64_t beginning : 63;
    uint64_t end;
    struct segment_list *next;
    struct segment_list *prev;
};

struct segment_list *create_head(void *memory, uint64_t, uint64_t);
uint64_t claim_next_free_segment(struct segment_list *);
void release_segment(uint64_t, struct segment_list *);
void print_segment_list(struct segment_list *);

#endif
