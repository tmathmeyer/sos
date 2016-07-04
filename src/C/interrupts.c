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

void load_IDT() {
    _load_IDT(IDT, sizeof(IDT));
}

void IDT_init() {
    for(int i=0;i<16;i++) {
        IDT[i] = create_empty();
    }
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
    return result;
}

inline uint16_t cs(void) {
    uint16_t val;
    asm volatile ( "mov %%cs, %0" : "=r"(val) );
    return val;
}
















/*
#define EOI() \
    __asm__ __volatile__ ( \
        "movb $0x20, %al\n\t" \
        "outb %al, $0x20\n\t" \
        "outb %al, $0xA0\n\t" \
    )

void outb(uint16_t port, uint8_t b) {
    __asm__ __volatile__ (
        "movw %0, %%dx\n\t"
        "movb %1, %%al\n\t"
        "outb %%al, %%dx\n\t"
        :
        : "m" (port), "m" (b)
        : "dx", "al"
    );
}

uint8_t inb(uint16_t port) {
    int result;
    __asm__ __volatile__ (
        "movw %0, %%dx\n\t"
        "inb %%dx, %%al\n\t"
        "movb %%al, %1\n\t"
        :
        : "m" (port), "m" (result)
        : "dx", "al"
    );

    return result;
}

void intel_8259_set_irq_mask(uint16_t mask) {
    outb(I8259_PORT_MASTER_DATA, mask & 0xFF);
    outb(I8259_PORT_SLAVE_DATA, (mask >> 8) & 0xFF);
}

uint16_t intel_8259_get_irq_mask() {
    uint8_t mask_lo = inb(I8259_PORT_MASTER_DATA);
    uint8_t mask_hi = inb(I8259_PORT_SLAVE_DATA);
    return (mask_hi << 8) | mask_lo;
}

void intel_8259_enable_irq(int irqno) {
    uint16_t mask = intel_8259_get_irq_mask();
    mask &= ~(uint16_t)(1 << irqno);
    intel_8259_set_irq_mask(mask);
}

void intel_8259_set_idt_start(int index) {
    uint16_t mask = intel_8259_get_irq_mask();

    outb(I8259_PORT_MASTER_COMMAND, I8259_ICW1_INIT + I8259_ICW1_ICW4);
    outb(I8259_PORT_SLAVE_COMMAND, I8259_ICW1_INIT + I8259_ICW1_ICW4);
    outb(I8259_PORT_MASTER_DATA, index);
    outb(I8259_PORT_SLAVE_DATA, index + 8);
    outb(I8259_PORT_MASTER_DATA, 1 << 2);
    outb(I8259_PORT_SLAVE_DATA, 2);
    outb(I8259_PORT_MASTER_DATA, I8259_ICW4_8086);
    outb(I8259_PORT_SLAVE_DATA, I8259_ICW4_8086);

    intel_8259_set_irq_mask(mask);
}


void keyboard(void) {
    kprintf("blah \n");
}



void set_handler(size_t at, void(fn)(void));
idt_entry_t idts[256];


void load_idt() {
    intel_8259_set_idt_start(32);
    memset(idts, 0, sizeof(idts));

    for(int i=32; i<256; i++) {
        set_handler(i, keyboard);
    }

    descriptor_table_t IDTR = {
        .offset = (uint64_t)idts,
        .limit  = sizeof(idts) - 1,
    };

    asm("lidt %0" :: "m" (IDTR) : "memory");
    asm("sti");
}

void set_handler(size_t at, void(_fn)(void)) {
    uint64_t fn = (uint64_t)_fn;
    idts[at].ptr_low  = fn & 0xffff;
    idts[at].ptr_mid  = (fn >> 16) & 0xffff;
    idts[at].ptr_high = (fn >> 32) & 0xffffffff;
    idts[at].zero = 0;
    idts[at]._zero = 0;
    idts[at].selector = 0x08;
    idts[at].type_attrs = 0xEE;
}
*/
