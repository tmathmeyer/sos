#include <sorts.h>
#include <alloc.h>
#include <libk.h>
#include <ktype.h>

bsl_t *new_bsl() {
    bsl_t *result = kmalloc(sizeof(bsl_t));
    result->__size = 0;
    result->__used = 0;
    result->__store = NULL;
    return result;
}

uint64_t bsl_add(bsl_t *bsl, void *item) {
    if (bsl->__size == bsl->__used) {
        uint64_t newsize = bsl->__size + 7;
        uint64_t strucsz = sizeof(void *) + sizeof(uint64_t);
        void *new_data = kmalloc(strucsz * newsize);
        if (bsl->__store) {
            memcpy(new_data, bsl->__store, strucsz * bsl->__size);
            kfree(bsl->__store);
        }
        bsl->__store = new_data;
        bsl->__size = newsize;
    }
    if (bsl->__used == 0) {
        bsl->__store[0].id = 1;
    } else {
        bsl->__store[bsl->__used].id = bsl->__store[bsl->__used-1].id+1;
    }
    bsl->__store[bsl->__used].data = item;
    bsl->__used ++;
    return bsl->__store[bsl->__used-1].id;
}

uint64_t _bsl_index(bsl_t *bsl, uint64_t id) {
    uint64_t s_min = 0;
    uint64_t s_max = bsl->__used;
    while(s_min < s_max) {
        uint64_t s_check = (s_min + s_max) / 2;
        uint64_t id_check = bsl->__store[s_check].id;
        if (id_check == id) {
            return s_check;
        }
        if (id_check < id) {
            s_min = s_check;
        }
        if (id_check > id) {
            s_max = s_check;
        }
    }
    uint64_t fail = 0;
    return ~fail;
}

void *bsl_remove(bsl_t *bsl, uint64_t id) {
    uint64_t index = _bsl_index(bsl, id);
    if (!~index) {
        return NULL;
    }
    void *res = bsl->__store[index].data;
    for(uint64_t i=index; i<bsl->__used-1; i++) {
        bsl->__store[i].id = bsl->__store[i+1].id;
        bsl->__store[i].data = bsl->__store[i+1].data;
    }
    bsl->__used --;
    return res;
}

void *bsl_get(bsl_t *bsl, uint64_t id) {
    uint64_t index = _bsl_index(bsl, id);
    if (!~index) {
        return NULL;
    }
    return bsl->__store[index].data;
}
