#ifndef kio_h
#define kio_h

#include "ktype.h"

static inline void outb(uint16_t port, uint8_t val){
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outs(uint16_t port, uint16_t val){
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outl(uint16_t port, uint32_t val){
    asm volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outsm(uint16_t port, uint8_t *data, uint64_t size) {
    asm volatile ("rep outsw" : "+S" (data), "+c" (size) : "d" (port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t ins(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int strncmp(char *a, char *b, size_t len);
uint64_t hex2int(char *x);
uint32_t backspace();
char *strfnd(char *, char);
void force_char(char, uint8_t, uint16_t, uint16_t);
void force_simple_string(char *, uint8_t, uint16_t, uint16_t);
uint32_t write_string(char *, uint8_t);
#endif
