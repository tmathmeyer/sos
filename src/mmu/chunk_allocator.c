#include "ktype.h"
#include "libk.h"
#include "chunk_allocator.h"

struct segment_list *create_head(void *memory, uint64_t start, uint64_t end) {
    struct segment_list *result = memory;
    result->allocated = 0;
    result->next = NULL;
    result->prev = NULL;
    result->beginning = start;
    result->end = end;
    return result;
}

void print_segment_list(struct segment_list *head) {
    int i = 0;
    kprintf("List Head = [%6ex]\n", head);
    while(head) {
        kprintf("#%03i = {.start=%03x, .end=%03x, .alloc=%03s}\n",
                i++, head->beginning, head->end, head->allocated?"alloc":"free");
        if (head->next && head->next->prev!=head) {
            kprintf("CONTINUITY BREAK!\n");
        }
        head = head->next;
    }
}

int is_empty(struct segment_list elem) {
    return elem.allocated == 0 &&
           elem.beginning == 0 &&
           elem.end == 0 &&
           elem.next == NULL &&
           elem.prev == NULL;
}

struct segment_list *find_next_empty(struct segment_list *head) {
    int i = 0;
    while(!is_empty(head[i])) {
        i++;
    }
    return &head[i];
}

uint64_t claim_next_free_segment(struct segment_list *head) {
    if (head->allocated) {
        //The first list element is allocated segments
        //This means that the next part of the list exists
        //contains free elements. We can just take one from there
        //collapse it if necessary, and return the new location
        if (!head->next) {
            //FAULT!!!!
        }
        uint64_t res = (head->end);
        head->end += 1;
        head->next->beginning += 1;
        if (head->next->beginning > head->next->end) {
            struct segment_list *skip = head->next->next;
            
            head->next->allocated = 0;
            head->next->beginning = 0;
            head->next->end = 0;
            head->next->next = NULL;
            head->next->prev = NULL;
            
            if (skip) {
                if (skip->next) {
                    skip->next->prev = head;
                    head->next = skip->next;
                    head->end = skip->next->beginning-1;
                }
                skip->allocated = 0;
                skip->beginning = 0;
                skip->end = 0;
                skip->prev = NULL;
                skip->next = NULL;
            } else {
                head->next = NULL;
            }
        }
        return res+1;
    } else {
        //The first list element is free'd things
        //This means one of two things:
        //   there is a [free][alloc'd][free]... pattern going on
        if (head->next) {
            uint64_t res = head->end;
            head->end -= 1;
            head->next->beginning -= 1;
            if (head->beginning > head->end) {
                head->allocated = 1;
                head->beginning = head->next->beginning;
                head->end = head->next->end;
                struct segment_list *next = head->next->next;
                head->next->allocated = 0;
                head->next->beginning = 0;
                head->next->end = 0;
                head->next->next = NULL;
                head->next->prev = NULL;
                head->next = next;
            }
            return res;
        }
        //   Or we're on the first allocation
        struct segment_list *next_segment = find_next_empty(head);
        next_segment->next = NULL;
        next_segment->prev = head;
        next_segment->allocated = 0;
        next_segment->beginning = head->beginning+1;
        next_segment->end = head->end;

        head->next = next_segment;
        head->allocated = 1;
        head->end = head->beginning;
        return head->beginning;
    }
}

int inside(uint64_t segment, struct segment_list *head) {
    return segment >= head->beginning &&
           segment <= head->end;
}

void release_segment(uint64_t segment, struct segment_list *head) {
    struct segment_list *found_in = head;
    while(!inside(segment, found_in)) {
        found_in = found_in->next;
    }
    if (!found_in->allocated) {
        //FAULT
    }

    if (found_in->beginning == segment) {
        if (found_in->prev) {
            found_in->prev->end += 1;
            found_in->beginning += 1;
            if (found_in->beginning > found_in->end) {
                if (found_in->next) {
                    struct segment_list *merge_me = found_in->next;
                    found_in->prev->next = merge_me->next;
                    found_in->prev->end = merge_me->end;
                    if (merge_me->next) {
                        merge_me->next = found_in->prev;
                    }
                    merge_me->next = NULL;
                    merge_me->prev = NULL;
                    merge_me->end = 0;
                    merge_me->beginning = 0;
                    merge_me->allocated = 0;
                    found_in->next = NULL;
                    found_in->prev = NULL;
                    found_in->end = 0;
                    found_in->beginning = 0;
                    found_in->allocated = 0;
                } else {
                    found_in->prev = found_in->next;
                    found_in->next = NULL;
                    found_in->prev = NULL;
                    found_in->end = 0;
                    found_in->beginning = 0;
                    found_in->allocated = 0;
                }
            }
        } else if (found_in == head) {
            struct segment_list *replacement = find_next_empty(head);
            replacement->next = head->next;
            replacement->prev = head;
            replacement->beginning = head->beginning+1;
            replacement->end = head->end;
            replacement->allocated = 1;
            if (replacement->next) {
                replacement->next->prev = replacement;
            }
            head->allocated = 0;
            head->next = replacement;
            head->prev = NULL;
            head->beginning = segment;
            head->end = segment;

            if (replacement->end < replacement->beginning) {
                struct segment_list *delete_me = replacement->next;
                if (delete_me) {
                    head->next = delete_me->next;
                    head->prev = NULL;
                    head->end = delete_me->end;
                    if (delete_me->next) {
                        delete_me->next->prev = head;
                    }
                    delete_me->end = 0;
                    delete_me->beginning = 0;
                    delete_me->next = NULL;
                    delete_me->prev = NULL;
                    replacement->end = 0;
                    replacement->beginning = 0;
                    replacement->next = NULL;
                    replacement->prev = NULL;
                }
            }
        }
    } else if (found_in->end == segment) {
        if(found_in->next) {
            found_in->next->beginning -= 1;
            found_in->end -= 1;
        } else {
            struct segment_list *next_segment = find_next_empty(head);
            next_segment->allocated = 0;
            next_segment->beginning = found_in->end;
            next_segment->end = found_in->end;
            next_segment->next = NULL;
            next_segment->prev = found_in;
            found_in->next = next_segment;
            found_in->end -= 1;
        }
    } else {
        struct segment_list *upper_half = find_next_empty(head);
        upper_half->allocated = 1;
        struct segment_list *free_section = find_next_empty(head);

        free_section->beginning = segment;
        free_section->end = segment;
        free_section->prev = found_in;
        free_section->next = upper_half;
        free_section->allocated = 0;

        upper_half->beginning = segment+1;
        upper_half->end = found_in->end;
        upper_half->prev = free_section;
        upper_half->next = found_in->next;
        upper_half->allocated = 1;

        if (upper_half->next) {
            upper_half->next->prev = upper_half;
        }

        found_in->next = free_section;
        found_in->end = segment-1;
    }
}
