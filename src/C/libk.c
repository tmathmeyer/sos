#include "libk.h"

void *memset(void *p, int b, size_t n) {
    char *_p = (char *)p;
    while(n --> 0) {
        *_p++ = b;
    }
    return p;
}

void wait(void) {
    for(int i=0;i<300000000;i++);
}
