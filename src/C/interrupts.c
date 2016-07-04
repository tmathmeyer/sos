#include "ktype.h"
#include "interrupts.h"
#include "libk.h"

idt_entry_t IDT[16];

static inline void _load_IDT(void* base, uint16_t size) {
    // This function works in 32 and 64bit mode
    struct {
        uint16_t length;
        void*    base;
    } __attribute__((packed)) IDTR = { size, base };
    kprintf("%4fs%4fx\n", "LIDT = ", &IDT);
    kprintf("%4fs%4fi\n", "CS = ", cs());
    asm ( "lidt %0" : : "m"(IDTR) );  // let the compiler choose an addressing mode
}

void _divide_by_zero_handler() {
    for(;;);
}

void IDT_init() {
    for(int i=0;i<16;i++) {
        IDT[i] = create_empty();
    }
}

void load_IDT() {
    IDT_init();
    set_handler(0x00, (uint64_t)&_divide_by_zero_handler);
    set_handler(0x08, (uint64_t)&_divide_by_zero_handler);
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
    result.opts.DPL         = 0;
    result.opts.gate_type   = 0x0C;
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
