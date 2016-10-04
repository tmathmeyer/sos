#include "ktype.h"
#include "interrupts.h"
#include "libk.h"
#include "kshell.h"
#include "kio.h"

// static memory for the interrupt descriptor table
idt_entry_t IDT[INTERRUPTS];

INT("divide by zero", 00) {
    kprintf("%6es divide by zero error\n", "[INT_HANDLER]");
    outb(PIC1, PIC_EOI);
}

INT("double fault", 08) {
    kprintf("%6es Double Fault!\n", "[INT_HANDLER]");
    outb(PIC1, PIC_EOI);
}

INT("triple fault", 0d) {
    kprintf("%6es Triple Fault!\n", "[INT_HANDLER]");
    while(1);
}

INT("keyboard", 21) {
    unsigned char status = inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        unsigned char keycode = inb(KEYBOARD_DATA_PORT);
        kshell(keycode);
    }
    outb(PIC1, PIC_EOI);
    outb(PIC2, PIC_EOI);
    return;
}




void IDT_set_zero() {
    for(int i=0;i<INTERRUPTS;i++) {
        IDT[i] = create_empty();
    }
}

static inline void _load_IDT(void* base, uint16_t size) {
    // This function works in 32 and 64bit mode
    struct {
        uint16_t length;
        void*    base;
    } __attribute__((packed)) IDTR = { size, base };
    asm("lidt %0" : : "m"(IDTR));  // let the compiler choose an addressing mode
    asm("sti");
}

void setup_IDT() {
    IDT_set_zero();

    // each of these declares an extern function (in ../asm/x86_64/long_mode.asm)
    // and then calls set_handler with 0x?? with the function pointer of the extern
    // set_handler puts the pointer info into an IDT struct and then adds it to the
    // IDT array. at the end of this function, that array is loaded using lidt
    HANDLE(00);
    HANDLE(08);
    HANDLE(0d);
    HANDLE(21);
}


void load_IDT() {
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
    outb(PIC1_DATA, 0xfd);
    outb(PIC2_DATA, 0xff);

    _load_IDT(IDT, sizeof(IDT)-1);
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

idt_entry_t create_empty() {
    idt_entry_t result;
    memset(&result, 0, sizeof(idt_entry_t));
    return result;
}
