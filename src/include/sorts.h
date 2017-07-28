#ifndef sorts_h
#define sorts_h

#include <ktype.h>

typedef
struct bsl_s {
    uint64_t __size;
    uint64_t __used;
    struct {
        void *data;
        uint64_t id;
    } *__store;
} bsl_t;

bsl_t *new_bsl();
uint64_t bsl_add(bsl_t *, void *);
void *bsl_remove(bsl_t *, uint64_t);
void *bsl_get(bsl_t *, uint64_t);
#endif
