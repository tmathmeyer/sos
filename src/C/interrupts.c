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

    asm ( "lidt %0" : : "m"(IDTR) );  // let the compiler choose an addressing mode
}

void divide_by_zero_handler() {
    for(;;);
}

void IDT_init() {
    for(int i=0;i<16;i++) {
        IDT[i] = create_empty();
    }
}

void load_IDT() {
    IDT_init();

    struct opts *options = set_handler(0, (uint64_t)&divide_by_zero_handler);

    _load_IDT(IDT, sizeof(IDT)-1);
}

struct opts *set_handler(uint8_t loc, size_t fn_ptr) {
    IDT[loc] = create(cs(), fn_ptr);
    return (struct opts *)&(IDT[loc].opts);
}

idt_entry_t create(uint16_t gdt_selector, size_t fn_ptr) {
    idt_entry_t result;
    result.selector = gdt_selector;
    result.ptr_low = (uint8_t)fn_ptr;
    result.ptr_mid = (uint8_t)(fn_ptr >> 16);
    result.ptr_high = (uint16_t)(fn_ptr >> 32);
    result.opts.ist_index   = 0;
    result.opts.reserved    = 0;
    result.opts.int_or_trap = 0;
    result.opts.must_be_one = 0x07;
    result.opts.must_be_zro = 0;
    result.opts.privelage_l = 0;
    result.opts.present     = 1;
    return result;
}

idt_entry_t create_empty() {
    idt_entry_t result;
    memset(&result, 0, sizeof(idt_entry_t));
    result.opts.must_be_one = 0x07;
    return result;
}

inline uint16_t cs(void) {
    uint16_t val;
    asm volatile ( "mov %%cs, %0" : "=r"(val) );
    return val;
}

//intel 8259 support later!
