#include "cpuio.h"

#define IDT_SIZE 256

struct IDT_entry IDT[IDT_SIZE];
extern void load_idt(void *);
extern uint16_t kernel_segment(void);
extern unsigned long keyboard_interrupt_handler;

struct IDT_ptr {
    uint16_t limit;
    uint64_t offset;
} __attribute__((packed));
