#ifndef libk_h
#define libk_h

#include "ktype.h"

#define ROWS 25
#define COLS 80
#define VIDB 2

enum vga_color {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN = 14,
    COLOR_WHITE = 15,
};

void wait(void);
void kio_init(void);
void clear_screen(void);
#define kprintf(...) _kprintf (0,0,0,0,0,0,0,0,0,0,__VA_ARGS__)
uint32_t _kprintf(int,int,int,int,int,int,int,int,int,int, const char fmt[], ...);
uint32_t kputs(char *);
#endif
