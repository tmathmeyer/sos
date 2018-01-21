#ifndef interrupts_h
#define interrupts_h

#include <std/int.h>

#define PIC1 0x20 // Master PIC
#define PIC2 0xA0 // Slave PIC
#define PIC1_DATA (PIC1+1)
#define PIC2_DATA (PIC2+1)
#define PIC_EOI 0x20 // end of interrupt
#define IRQ_BASE 0x20

#define INTERRUPTS 256

struct interrupt_frame {
    uint64_t instruction_ptr;
    uint64_t code_segment;
    uint64_t cpu_flags;
    uint64_t stack_pointer;
    uint64_t stack_segment;
}__attribute__((packed));

struct opts {
    uint8_t stack_OK  : 3;
    uint8_t ZEROS     : 5;
    uint8_t gate_type : 1;
    uint8_t ONES      : 3;
    uint8_t ZERO      : 1;
    uint8_t DPL       : 2;
    uint8_t present   : 1;
}__attribute__((packed));

typedef struct idt_entry {
    uint16_t ptr_low;
    uint16_t selector;
    struct opts opts;
    uint16_t ptr_mid;
    uint32_t ptr_high;

    uint8_t  _1_reserved : 8;
    uint8_t  _type       : 5;
    uint32_t _2_reserved : 19;
}__attribute__((packed)) idt_entry_t;

idt_entry_t create(uint16_t, uint64_t);
void setup_IDT();
void set_interrupt_handler(int handler_id, void (* func) (struct interrupt_frame *));
uint64_t get_error_code(struct interrupt_frame *);

extern void irq_0(void);
extern void irq_1(void);
extern void irq_2(void);
extern void irq_3(void);
extern void irq_4(void);
extern void irq_5(void);
extern void irq_6(void);
extern void irq_7(void);
extern void irq_8(void);
extern void irq_9(void);
extern void irq_10(void);
extern void irq_11(void);
extern void irq_12(void);
extern void irq_13(void);
extern void irq_14(void);
extern void irq_15(void);
extern void irq_16(void);
extern void irq_17(void);
extern void irq_18(void);
extern void irq_19(void);
extern void irq_20(void);
extern void irq_21(void);
extern void irq_22(void);
extern void irq_23(void);
extern void irq_24(void);
extern void irq_25(void);
extern void irq_26(void);
extern void irq_27(void);
extern void irq_28(void);
extern void irq_29(void);
extern void irq_30(void);
extern void irq_31(void);
extern void irq_32(void);
extern void irq_33(void);
extern void irq_34(void);

#endif
