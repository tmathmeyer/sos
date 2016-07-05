#include "ktype.h"
#include "interrupts.h"
#include "libk.h"

#define INTERRUPTS 256
#define HANDLE(i) do { \
    extern void interrupt_handler_##i(); \
    set_handler(0x##i, (uint64_t)interrupt_handler_##i); \
} while(0)

#define INT(name, num) void _interrupt_handler_##num()

idt_entry_t IDT[INTERRUPTS];


INT("divide by zero", 00) {
    int j;
    kprintf("divide by zero error\n");
    kprintf("stack is at: %f3x\n", &j);
    return;
}

INT("double fault", 08) {
    kprintf("%3fs\n", "caught a double fault!\n");
    while(1);
}

INT("keyboard", 01) {
    kprintf("keyboard!");
}




extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);


void IDT_init() {
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
    asm ( "lidt %0" : : "m"(IDTR) );  // let the compiler choose an addressing mode
}

void load_IDT() {
    IDT_init();
    HANDLE(00);
    HANDLE(08);
    HANDLE(01);

    write_port(0x20 , 0x11);
    write_port(0xA0 , 0x11);
    write_port(0x21 , 0x20);
    write_port(0xA1 , 0x28);
    write_port(0x21 , 0x00);
    write_port(0xA1 , 0x00);
    write_port(0x21 , 0x01);
    write_port(0xA1 , 0x01);
    write_port(0x21 , 0xff);
    write_port(0xA1 , 0xff);

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

inline uint16_t cs(void) {
    uint16_t val;
    asm volatile ( "mov %%cs, %0" : "=r"(val) );
    return val;
}

//intel 8259 support later!
