#ifndef libk_h
#define libk_h

#include "ktype.h"

#define ROWS 25
#define COLS 80
#define VIDB 2

#define PAGE 1
#define FRAME 0

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

#define min(x, y) (((x)>(y))?(y):(x))
#define max(a, b) (((a)>(b))?(a):(b))

void *memset(void *p, int b, size_t n);
void *memcpy(void *dest, void *src, size_t bytes);
uint64_t strlen(char *c);
uint64_t stringlen(char *);


void wait(void);
void kio_init(void);
void clear_screen(void);
#define sprintf(...) _sprintf (0,0,0,0,0,0,0,0,0,0,__VA_ARGS__)
uint32_t _sprintf(int,int,int,int,int,int,int,int,int,int, char *dest, const char fmt[], ...);
#define kprintf(...) _kprintf (0,0,0,0,0,0,0,0,0,0,__VA_ARGS__)
uint32_t _kprintf(int,int,int,int,int,int,int,int,int,int, const char fmt[], ...);
uint32_t kputs(char *);

#define WARN(msg) do{warn_msg((msg), __FILE__, __LINE__);}while(0)
static inline void warn_msg(char *msg, char *file, uint32_t line_no) {
    kprintf("%6fs%6fs%6fi%6fs%6fs\n", file, "(", line_no, ") WARN: ", msg);
}

#define ERROR(msg) do{error_stack_dump((msg), __FILE__, __LINE__);}while(0)
static inline void error_stack_dump(char *msg, char *file, uint32_t line_no) {
    kprintf("%cfs%4fs\n%4fs:%4fx\n", "ERROR: ", msg, file, line_no);
}

#endif
