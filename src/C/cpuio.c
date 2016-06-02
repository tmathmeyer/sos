#include "cpuio.h"

#define IDT_SIZE 256

struct IDT_entry IDT[IDT_SIZE];
extern void load_idt(void *);
extern uint16_t kernel_segment(void);
extern unsigned long keyboard_interrupt_handler;

void keyboard_handler(void) {
    kprintf("fuck...");
}

struct IDT_ptr {
    uint16_t limit;
    uint64_t offset;
} __attribute__((packed));


void initialize_keyboard(void) {
    uint64_t keyboard_address = (uint64_t)keyboard_interrupt_handler;
    struct IDT_entry keyboard_interrupt = {
        .selector    = kernel_segment(),
        .low_bits    = (uint16_t)keyboard_address,
        .middle_bits = (uint16_t)(keyboard_address >> 16),
        .high_bits   = (uint32_t)(keyboard_address >> 32),
        .flags       = INTERRUPT_GATE,
        .zero        = 0,
        ._reserved   = 0
    };

    IDT[0x21] = keyboard_interrupt;
    struct IDT_ptr idt_ptr = {
        .offset = (uint64_t) IDT,
        .limit = (sizeof(struct IDT_entry) * IDT_SIZE) - 1
    };

    load_idt(&idt_ptr);
    //asm volatile("lidt ($0)" :: "r" (&idt_ptr) : "memory");
}

void initialize_interrupts(void) {
    char pic1_mask = inb(PIC1_DATA);
    char pic2_mask = inb(PIC2_DATA);

    // tell each PIC that it will be initialized with a 3 byte struct
    outb(CMD_INIT, PIC1_COMMAND);
    io_delay();
    outb(CMD_INIT, PIC2_COMMAND);
    io_delay();

    // write each proper offset (x86_64 = {32, 40})
    outb(OFFSET1, PIC1_DATA);
    io_delay();
    outb(OFFSET2, PIC2_DATA);
    io_delay();

    // chain the PIC's with magic bits
    outb(4, PIC1_DATA);
    io_delay();
    outb(2, PIC2_DATA);
    io_delay();

    // set PIC's to some super_sayan_mode
    outb(MODE_8086, PIC1_DATA);
    io_delay();
    outb(MODE_8086, PIC2_DATA);
    io_delay();

    // hmmmmmmmmm these should be pic1_mask, but im not sure what is wrong with it!
    outb(0xff, PIC1_DATA);
    io_delay();
    outb(0xff, PIC2_DATA);
    io_delay();
}
