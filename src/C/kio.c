#include "libk.h"

char *VGA = (char *)0xb8000;
uint8_t X;
uint8_t Y;



static inline void outb(unsigned short port, unsigned char val){
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

void update_cursor(){
    unsigned short position=(Y*COLS + X);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position&0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char )((position>>8)&0xFF));
}

void kio_init(void) {
    Y = (ROWS-1);
    X = 0;
    update_cursor();
}

void clear_screen(void) {
    unsigned int i = 0;
    while (i < ROWS * COLS * VIDB) {
        VGA[i++] = ' ';
        VGA[i++] = 0x07;
    }
}

void mm_copy(void *dest, void *src, uint64_t bytes) {
    char *d = (char *)dest;
    char *s = (char *)src;
    for(uint64_t i=0;i<bytes;i++) {
        d[i] = s[i];
    }
}

void scroll(void) {
    mm_copy(VGA, VGA+(COLS * VIDB), (ROWS-1) * COLS * VIDB);
    char *write = VGA+(ROWS-1)*VIDB*COLS;
    for(int i=0;i<COLS;i++){
        write[2*i] = ' ';
        write[2*i+1] = 0x07;
    }
    X = 0;
}

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

uint32_t write_char(char str, uint8_t color) {
    uint32_t l = X + Y*COLS;
    l *= VIDB;
    VGA[l++] = str;
    VGA[l++] = color;
    if (++X==COLS) {
        scroll();
    }
    return 1;
}

uint32_t write_hex(uint64_t a, uint8_t color) {
    uint8_t writing = 1;
    write_char('0', color);
    write_char('x', color);

    char *chars = "0123456789ABCDEF";
    uint8_t shift = 64;
    do {
        shift -= 4;
        uint8_t write = (a>>shift)&0x0F;
        if (write || writing) {
            writing++;
            write_char(chars[(a>>shift)&0x0F], color);
        }
    } while(shift);
    return writing+1;
}


uint32_t write_string(char *str, uint8_t color) {
    uint32_t total = 0;
    while(*str != 0) {
        switch(*str) {
            case '\n':
                scroll();
                break;

            default:
                total += write_char(*str, color);
        }
        str++;
    }
    return total;
}

uint32_t write_num(uint64_t num, uint8_t color) {
    uint8_t digits = 0;
    char nums[25];
    do {
        nums[digits++] = (num%10) + '0';
        num /= 10;
    } while(num);
    uint8_t d_r = digits;
    while(digits --> 0) {
        write_char(nums[digits], color);
    }
    return d_r;
}

uint32_t _kprintf(int a1,int a2,int a3,int a4,int a5,int a6,int a7,int a8,int a9,int a0, const char fmt[], ...) {
    int chars = 0;
    void **arg;
    char c;
    arg = (void **) &fmt;
    arg++;

    c = *fmt;
    while (c) {
        switch (c) {
            case '\b':
                break;
            case '\r':
                clear_screen();
                break;
            case '\n':
                scroll();
                break;
            case '%':
                fmt++;
                uint8_t color = hexr(*fmt++)<<4 | hexr(*fmt++);
                c = *fmt;
                if (arg) {
                    if (c) {
                        char temp[10];
                        switch(c) {
                            case 's':
                                chars += write_string(*((char **) arg), color);
                                break;

                            case 'c':
                                write_char(*((char *)arg), color);
                                chars++;
                                break;

                            case 'n':
                                chars += write_num(*((uint64_t *) arg), color);
                                break;

                            case 'x':
                                chars += write_hex(*((uint64_t *) arg), color);
                                break;
                        }
                        arg++;
                    }
                }
                break;
            default:
                write_char(c, 0x07);
                chars++;
        }
        if (c) {
            fmt++;
            c = *fmt;
        }
    }
    return chars;
}
