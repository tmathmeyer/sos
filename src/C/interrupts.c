#include "ktype.h"
#include "interrupts.h"
#include "libk.h"
#include "kshell.h"
#include "kio.h"
#include "registers.h"
#include "timer.h"

// static memory for the interrupt descriptor table
idt_entry_t IDT[INTERRUPTS];
void (*handlers[INTERRUPTS]) (void);
void (*irqs[INTERRUPTS]) (void) = {
    &irq_0, &irq_1, &irq_2, &irq_3, &irq_4, &irq_5, &irq_6, &irq_7, &irq_8,
    &irq_9, &irq_10, &irq_11, &irq_12, &irq_13, &irq_14, &irq_15, &irq_16,
    &irq_17, &irq_18, &irq_19, &irq_20, &irq_21, &irq_22, &irq_23, &irq_24,
    &irq_25, &irq_26, &irq_27, &irq_28, &irq_29, &irq_30, &irq_31, &irq_32,
    &irq_33,
};

void invalid_opcode(void) {
    kprintf("INVALID OPCODE");
    while(1);
}

void divide_by_zero(void) {
    kprintf("%6es divide by zero error\n", "[INT_HANDLER]");
    while(1);
}

void double_fault(void) {
    kprintf("%6es Double Fault!\n", "[INT_HANDLER]");
    while(1);
}

void triple_fault(void) {
    kprintf("%6es Triple Fault!\n", "[INT_HANDLER]");
    while(1);
}

void keyboard(void) {
    unsigned char status = inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        unsigned char keycode = inb(KEYBOARD_DATA_PORT);
        kshell(keycode);
    }
}

void setup_IDT() {
    // start initialization
    outb(PIC1, 0x11);
    outb(PIC2, 0x11);

    // set IRQ base numbers for each PIC
    outb(PIC1_DATA, IRQ_BASE);
    outb(PIC2_DATA, IRQ_BASE+8);

    // use IRQ number 2 to relay IRQs from the slave PIC
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    // finish initialization
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // mask all IRQs
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);

    for(int i=0;i<INTERRUPTS;i++) {
        IDT[i] = create(cs(), (uint64_t)irqs[i]);
        handlers[i] = 0;
    }

    struct {
        uint16_t length;
        void*    base;
    } __attribute__((packed)) IDTR = {sizeof(IDT)-1, IDT};
    asm("lidt %0" : : "m"(IDTR));  // let the compiler choose an addressing mode
    asm("sti");

    set_interrupt_handler(0x08, double_fault);
    set_interrupt_handler(0x06, invalid_opcode);
    set_interrupt_handler(0x0d, triple_fault);
    set_interrupt_handler(0x00, divide_by_zero);

    outb(PIC1_DATA, 0xfd);
    outb(PIC2_DATA, 0xff);
    set_interrupt_handler(0x21, keyboard);
}

void common_handler(uint64_t id) {
    if (handlers[id]) {
        handlers[id]();
    } else {
        kprintf("unhandled exception: %02i\n", id);
    }
    if (id >= 0x28 && id < 0x30) outb(PIC2, PIC_EOI);
    if (id >= 0x20 && id < 0x30) outb(PIC1, PIC_EOI);
}

void set_interrupt_handler(int handler_id, void (* func) (void)) {
    handlers[handler_id] = func;
}

struct opts *set_handler(uint8_t loc, uint64_t fn_ptr) {
    IDT[loc] = create(cs(), fn_ptr);
    return (struct opts *)&(IDT[loc].opts);
}

idt_entry_t create(uint16_t gdt_selector, uint64_t fn_ptr) {
    idt_entry_t result;
    result.selector = gdt_selector;
    result.ptr_low = (uint16_t)fn_ptr;
    result.ptr_mid = (uint16_t)(fn_ptr >> 16);
    result.ptr_high = (uint32_t)(fn_ptr >> 32);

    result.opts.present     = 1;
    result.opts.DPL         = 3;
    result.opts.gate_type   = 0x0F;
    result.opts.ZERO        = 0;
    result.opts.ZEROS       = 0;

    result._1_reserved = 0;
    result._2_reserved = 0;
    result._type = 0;
    return result;
}
