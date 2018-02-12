#ifndef kshell_h
#define kshell_h

#include <std/int.h>

#define RDL_SIZE 256

void kshell(unsigned char key);
int strncmp(char *a, char *b, size_t len);
uint64_t hex2int(char *x);
uint32_t backspace();
char *strfnd(char *, char);
void force_char(char, uint8_t, uint16_t, uint16_t);
void force_simple_string(char *, uint8_t, uint16_t, uint16_t);
uint32_t write_string(char *, uint8_t);

uint64_t stringlen(char *);


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

#define DBGME() do {kprintf("%9cs:%9ci\n", __func__, __LINE__);}while(0)

#endif
