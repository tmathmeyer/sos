
#ifndef kio_h
#define kio_h


static inline void outb(unsigned short port, unsigned char val){
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint16_t cs();
int strncmp(char *a, char *b, size_t len);
uint64_t hex2int(char *x);
uint32_t backspace();
char *strfnd(char *, char);
#endif
