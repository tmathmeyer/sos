#include "libk.h"
#include "mmu.h"

void *memset(void *p, int b, size_t n) {
    char *_p = (char *)p;
    while(n --> 0) {
        *_p++ = b;
    }
    return p;
}

void wait(void) {
    for(int i=0;i<10000000;i++);
}

void *memcpy(void *_dest, void *_src, size_t bytes) {
    char *dest = (char *)_dest;
    char *src = (char *)_src;
    while(bytes --> 0) {
        *dest++ = *src++;
    }
}

uint64_t strlen(char *c) {
    uint64_t res = 0;
    while(*c) {
        res++;
        c++;
    }
    return res;
}

uint64_t stringlen(char *c) {
    uint64_t i = 0;
    if (c) {
        while(c[i]) {
            i++;
        }
    }
    return i;
}
