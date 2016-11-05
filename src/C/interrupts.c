#include "ktype.h"
#include "interrupts.h"
#include "libk.h"
#include "kshell.h"
#include "kio.h"
#include "registers.h"
#include "scheduler.h"

INT("divide by zero", 0x00) {
    kprintf("%6es divide by zero error\n", "[INT_HANDLER]");
    outb(PIC1, PIC_EOI);
}
INT("double fault", 0x08) {
    kprintf("%6es Double Fault!\n", "[INT_HANDLER]");
    while(1);
}

INT("triple fault", 0x0d) {
    kprintf("%6es Triple Fault!\n", "[INT_HANDLER]");
    while(1);
}

uint64_t sched = 0;
INT("timer", 0x20) {
    sched++; // expect wrap around!
    if (!(sched&0xFFFF)) {
        reschedule();
    }
    outb(PIC1, PIC_EOI);
    outb(PIC2, PIC_EOI);
    return;
}

uint8_t r=0, w=0;
char b[256] = {0};

void write(char c) {
    b[w] = c;
    w+=1;
    if (w == r) {
        r++;
    }
}

char read() {
    if (r != w) {
        uint8_t i = r;
        r+=1;
        return b[i];
    }
    return 0;
}

INT("keyboard", 0x21) {
    unsigned char status = inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        unsigned char keycode = inb(KEYBOARD_DATA_PORT);
        write(keycode);
    }
    return;
}









typedef void (*local_idt_handler_t)(struct idt_data*);
typedef void (*extern_idt_handler_t)(void);
extern_idt_handler_t isr_vector[] = {
     &isr_0, &isr_1, &isr_2, &isr_3, &isr_4, &isr_5, &isr_6, &isr_7, &isr_8,
     &isr_9, &isr_10, &isr_11, &isr_12, &isr_13, &isr_14, &isr_15, &isr_16,
     &isr_17, &isr_18, &isr_19, &isr_20, &isr_21, &isr_22, &isr_23, &isr_24,
     &isr_25, &isr_26, &isr_27, &isr_28, &isr_29, &isr_30, &isr_31, &isr_32,
     &isr_33, &isr_34, &isr_35, &isr_36, &isr_37, &isr_38, &isr_39, &isr_40,
     &isr_41, &isr_42, &isr_43, &isr_44, &isr_45, &isr_46, &isr_47
};

// static memory for the interrupt descriptor table
idt_entry_t         IDT[INTERRUPTS];
local_idt_handler_t idt_handlers[INTERRUPTS];



void setup_IDT(void) {
    idt_handlers[0x21] = _interrupt_handler_0x21;

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

    // initialize the IDT elements to the ones from long_mode.asm
    // initialize the handlers to 0
    for (int i = 0; i < INTERRUPTS; i++) {
        set_handler(i, (uint64_t)isr_vector[i]);
        idt_handlers[i] = 0;
    }

    // first we load the IDT with __asm__(lidt)
    struct {
        uint16_t length;
        void*    base;
    } __attribute__((packed)) IDTR = {sizeof(IDT)-1, IDT};
    asm("lidt %0" : : "m"(IDTR));  // let the compiler choose an addressing mode
    asm("sti");
}

static void fallback_handler(struct idt_data* data) {
    kprintf("## Received Interrupt %02x ##\n", data->int_no);
}

void idt_handler(struct idt_data* data)
{
	/* Use the given interrupt handler if exists or use the fallback. */
	if (idt_handlers[data->int_no] != 0) {
		idt_handlers[data->int_no](data);
	} else {
		fallback_handler(data);
	}

	/* If this is an exception, I have to halt the system (I think?) */
	if (data->int_no < 16) while(1);

	/* Acknowledge the interrupt to the master PIC and possibly slave PIC. */
	if (data->int_no >= 0x28 && data->int_no < 0x30) outb(0xA0, 0x20);
	if (data->int_no >= 0x20 && data->int_no < 0x30) outb(0x20, 0x20);
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
