
#ifndef kio_h
#define kio_h

#include "ktype.h"

static inline void outb(unsigned short port, unsigned char val){
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int strncmp(char *a, char *b, size_t len);
uint64_t hex2int(char *x);
uint32_t backspace();
char *strfnd(char *, char);
void force_char(char, uint8_t, uint16_t, uint16_t);
void force_simple_string(char *, uint8_t, uint16_t, uint16_t);
#endif
