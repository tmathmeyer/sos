#include <std/int.h>
#include <std/string.h>

char hexr(char c) {
    switch(c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': return 0xa;
        case 'b': return 0xb;
        case 'c': return 0xc;
        case 'd': return 0xd;
        case 'e': return 0xe;
        case 'f': return 0xf;
        case 'A': return 0xa;
        case 'B': return 0xb;
        case 'C': return 0xc;
        case 'D': return 0xd;
        case 'E': return 0xe;
        case 'F': return 0xf;
    }
}

int strncmp(char *a, char *b, size_t len) {
    while(len-- && *a && *b) {
        if (*a != *b) {
            return *a - *b;
        }
        b++;
        a++;
    }
    if (len != -1) {
        return *a-*b;
    }
    return 0;
}

char *strfnd(char *str, char fnd) {
    while(*str && *str!=fnd) {
        str++;
    }
    if (str) {
        return str;
    }
    return (char *)0;
}

uint64_t hex2int(char *x) {
    uint64_t res = 0;
    while(*x && ((*x>='0'&&*x<='9')||(*x>='A'&&*x<='Z')||(*x>='a'&&*x<='z'))) {
        res *= 0x10;
        res += hexr(*x);
        x++;
    }
    return res;
}

uint64_t strlen(char *c) {
    uint64_t res = 0;
    while(*c) {
        res++;
        c++;
    }
    return res;
}

void *memcpy(void *_dest, void *_src, size_t bytes) {
    char *dest = (char *)_dest;
    char *src = (char *)_src;
    while(bytes --> 0) {
        *dest++ = *src++;
    }
}

void *memset(void *p, int b, size_t n) {
    char *_p = (char *)p;
    while(n --> 0) {
        *_p++ = b;
    }
    return p;
}