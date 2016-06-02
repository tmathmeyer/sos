#include "libk.h"

int print_place = 2;
void kmemcopy(char *src, char *dest, unsigned long bytes) {
    while(bytes --> 0) {
        *dest = *src;
        dest++;
        src++;
    }
    return;
}

void wait(void) {
    for(int i=0;i<300000000;i++);
}
