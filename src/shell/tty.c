#include <shell/tty.h>
#include <arch/io.h>
#include <std/string.h>
#include <std/int.h>

char *VGA = (char *)0xb8000;
uint8_t X;
uint8_t Y;


void force_char(char c, uint8_t color, uint16_t x, uint16_t y) {
    uint32_t l = x + y*COLS;
    l *= VIDB;
    VGA[l++] = c;
    VGA[l++] = color;
}

void force_simple_string(char *str, uint8_t color, uint16_t x, uint16_t y) {
    for(int i=x; (x<COLS)&&*str; i++) {
        force_char(*str++, color, i, y);
    }
}

void update_cursor(){
    unsigned short position=(Y*COLS + X);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position&0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char )((position>>8)&0xFF));
}

void clear_screen(void) {
    unsigned int i = 0;
    while (i < ROWS * COLS * VIDB) {
        VGA[i++] = ' ';
        VGA[i++] = 0x07;
    }
}

void kio_init(void) {
    Y = (ROWS-1);
    X = 0;
    update_cursor();
}

void scroll(void) {
    memcpy(VGA, VGA+(COLS * VIDB), (ROWS-1) * COLS * VIDB);
    char *write = VGA+(ROWS-1)*VIDB*COLS;
    for(int i=0;i<COLS;i++){
        write[2*i] = ' ';
        write[2*i+1] = 0x07;
    }
    X = 0;
}

uint32_t backspace() {
    if (X == 0) {
        X=COLS;
        Y--;
    } else {
        X--;
    }
    uint32_t l = X + Y*COLS;
    l *= VIDB;
    VGA[l++] = ' ';
    VGA[l++] = 0x03;
    return 1;
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

uint32_t write_cl_hex(uint64_t a, uint8_t color) {
    uint8_t b = a;
    char *chars = "0123456789ABCDEF";
    write_char(chars[b>>4], color);
    write_char(chars[b&0xF], color);
    return 2;
}

uint32_t write_hex(uint64_t a, uint8_t color) {
    uint8_t writing = 0;
    write_char('0', color);
    write_char('x', color);
    if (a==0) {
        write_char('0', color);
        return 3;
    }

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
    return writing+2;
}

uint32_t write_le_hex(uint64_t a, uint8_t color) {
    uint32_t res = a;
    for(int i=0; i<0; i++) {
        res<<=8;
        res += (a&0xff);
        a>>=8;
    }
    return write_hex(res, color);
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

                            case 'L':
                                chars += write_le_hex(*((uint64_t *) arg), color);
                                break;

                            case 'X':
                                chars += write_cl_hex(*((uint64_t *) arg), color);
                                break;

                            case 'i':
                                chars += write_num(*((uint64_t *) arg), color);
                                break;

                            case 'b':
                                chars += write_char(*((uint8_t *)arg)?'1':'0', color);
                                chars++;
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
