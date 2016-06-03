#ifndef cpuio_h
#define cpuio_h

#include "libk.h"

#define PIC1            0x20        /* IO base address for master PIC */
#define PIC2            0xA0        /* IO base address for slave PIC */
#define PIC1_COMMAND    (PIC1)
#define PIC2_COMMAND    (PIC2)
#define PIC_EOI         0x20
#define INTERRUPT_GATE  0x8e

struct IDT_entry {
    uint16_t low_bits;
    uint16_t selector;
    union {
        uint16_t options;
        struct {
            uint8_t zero;
            uint8_t flags;
        };
    };
    uint16_t middle_bits;
    uint32_t high_bits;
    uint32_t _reserved;
}__attribute__((packed));

static inline void outb(unsigned char v, unsigned short port) {
    asm volatile("outb %0,%1" : : "a" (v), "dN" (port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char v;
    asm volatile("inb %1,%0" : "=a" (v) : "dN" (port));
    return v;
}

static inline void outw(unsigned short v, unsigned short port) {
    asm volatile("outw %0,%1" : : "a" (v), "dN" (port));
}

static inline unsigned short inw(unsigned short port) {
    unsigned short v;
    asm volatile("inw %1,%0" : "=a" (v) : "dN" (port));
    return v;
}

static inline void outl(unsigned int v, unsigned short port) {
    asm volatile("outl %0,%1" : : "a" (v), "dN" (port));
}

static inline unsigned int inl(unsigned short port) {
    unsigned int v;
    asm volatile("inl %1,%0" : "=a" (v) : "dN" (port));
    return v;
}

static inline void io_delay(void) {
    const unsigned short DELAY_PORT = 0x80;
    asm volatile("outb %%al,%0" : : "dN" (DELAY_PORT));
}

static inline void PIC_sendEOI(unsigned char irq) {
    if(irq >= 8) {
        outb(PIC2_COMMAND,PIC_EOI);
    }
    outb(PIC1_COMMAND,PIC_EOI);
}

#endif
