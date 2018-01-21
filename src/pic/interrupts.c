#include <std/int.h>
#include <pic/interrupts.h>
#include <arch/io.h>
#include <shell/shell.h>
#include <arch/lock.h>
#include <arch/registers.h>

// static memory for the interrupt descriptor table
idt_entry_t IDT[INTERRUPTS];
void (*handlers[INTERRUPTS]) (struct interrupt_frame *);
void (*irqs[INTERRUPTS]) (void) = {
    &irq_0, &irq_1, &irq_2, &irq_3, &irq_4, &irq_5, &irq_6, &irq_7, &irq_8,
    &irq_9, &irq_10, &irq_11, &irq_12, &irq_13, &irq_14, &irq_15, &irq_16,
    &irq_17, &irq_18, &irq_19, &irq_20, &irq_21, &irq_22, &irq_23, &irq_24,
    &irq_25, &irq_26, &irq_27, &irq_28, &irq_29, &irq_30, &irq_31, &irq_32,
    &irq_33,
};

struct interrupt_frame *get_real_frame(uint64_t frame_addr, uint64_t id) {
    switch (id) {
        case 8: case 10: case 11: case 12:
        case 13: case 14: case 17: case 30:
            // account for the error code
            frame_addr += sizeof(uint64_t);
    }
    struct interrupt_frame *above = (struct interrupt_frame *)frame_addr;
    return above+3;
}

uint64_t get_error_code(struct interrupt_frame *frame) {
    uint64_t *framew = (uint64_t *)frame;
    return framew[-1];
}


void dump_frame(struct interrupt_frame *frame) {
    kprintf("    InsPtr = %6ex\n", frame->instruction_ptr);
    kprintf("    CodSeg = %6ex\n", frame->code_segment);
    kprintf("    CPUFlg = %6ex\n", frame->cpu_flags);
    kprintf("    StkPtr = %6ex\n", frame->stack_pointer);
    kprintf("    StkSgm = %6ex\n", frame->stack_segment);
}

void invalid_opcode(struct interrupt_frame *frame) {
    kprintf("%6es Invalid Opcode\n", "[INT_HANDLER]");
    dump_frame(frame);
    while(1);
}

void divide_by_zero(struct interrupt_frame *frame) {
    kprintf("%6es divide by zero error\n", "[INT_HANDLER]");
    dump_frame(frame);
    while(1);
}

void double_fault(struct interrupt_frame *frame) {
    kprintf("%6es Double Fault!\n", "[INT_HANDLER]");
    dump_frame(frame);
    while(1);
}

void triple_fault(struct interrupt_frame *frame) {
    kprintf("%6es Triple Fault!\n", "[INT_HANDLER]");
    dump_frame(frame);
    while(1);
}

void page_fault(struct interrupt_frame *frame) {
    kprintf("page while accessing virtual address: <<unknown>>\n");
    dump_frame(frame);
    kprintf("    ErrCod = %6ex\n", get_error_code(frame));
    while(1);
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

    set_interrupt_handler(0x06, invalid_opcode);
    set_interrupt_handler(0x08, double_fault);
    set_interrupt_handler(0x0d, triple_fault);
    set_interrupt_handler(0x0e, page_fault);
    set_interrupt_handler(0x00, divide_by_zero);
    asm("sti");
}

lock_t interrupt_lock;
void common_handler(uint64_t id, uint64_t stack) {
    if (try_lock(&interrupt_lock)) {
        if (handlers[id]) {
            handlers[id](get_real_frame(stack, id));
        } else {
            kprintf("unhandled exception: %02i\n", id);
        }
        if (id >= 0x28 && id < 0x30) outb(PIC2, PIC_EOI);
        if (id >= 0x20 && id < 0x30) outb(PIC1, PIC_EOI);
        spin_unlock(&interrupt_lock);
    }
}

void set_interrupt_handler(int handler_id, void (* func) (struct interrupt_frame *)) {
    handlers[handler_id] = func;
}

idt_entry_t create(uint16_t gdt_selector, uint64_t fn_ptr) {
    idt_entry_t result;
    result.selector = gdt_selector;
    result.ptr_low = (uint16_t)fn_ptr;
    result.ptr_mid = (uint16_t)(fn_ptr >> 16);
    result.ptr_high = (uint32_t)(fn_ptr >> 32);

    result.opts.stack_OK    = 0; // do not switch stack
    result.opts.present     = 1; // are we valid
    result.opts.DPL         = 3; // priv to call int handler
    result.opts.gate_type   = 0x01; // 1 = interrupt, 2 = trap
    result.opts.ONES        = 0x07;
    result.opts.ZERO        = 0;
    result.opts.ZEROS       = 0;

    result._1_reserved = 0;
    result._2_reserved = 0;
    result._type = 0;
    return result;
}
