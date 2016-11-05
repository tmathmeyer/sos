#ifndef interrupts_h
#define interrupts_h

#include "ktype.h"

#define PIC1 0x20 // Master PIC
#define PIC2 0xA0 // Slave PIC
#define PIC1_DATA (PIC1+1)
#define PIC2_DATA (PIC2+1)
#define PIC_EOI 0x20 // end of interrupt
#define IRQ_BASE 0x20
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_DATA_PORT 0x60

#define INTERRUPTS 256
#define HANDLE(i) do { \
    extern void interrupt_handler_##i(); \
    set_handler(i, (uint64_t)interrupt_handler_##i); \
} while(0)

#define INT(name, num) void _interrupt_handler_##num()

struct opts {
    uint8_t ZEROS     : 8;
    uint8_t gate_type : 4;
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

idt_entry_t create_empty();
idt_entry_t create(uint16_t, uint64_t);
struct opts *set_handler(uint8_t loc, uint64_t fn_ptr);
void load_IDT();
void setup_IDT();
char read();


struct idt_data {
    unsigned int edi, esi, ebp, esp;
    unsigned int eax, ebx, ecx, edx;
    unsigned int int_no, err_code;
} __attribute__((packed));

extern void isr_0(void);
extern void isr_1(void);
extern void isr_2(void);
extern void isr_3(void);
extern void isr_4(void);
extern void isr_5(void);
extern void isr_6(void);
extern void isr_7(void);
extern void isr_8(void);
extern void isr_9(void);
extern void isr_10(void);
extern void isr_11(void);
extern void isr_12(void);
extern void isr_13(void);
extern void isr_14(void);
extern void isr_15(void);
extern void isr_16(void);
extern void isr_17(void);
extern void isr_18(void);
extern void isr_19(void);
extern void isr_20(void);
extern void isr_21(void);
extern void isr_22(void);
extern void isr_23(void);
extern void isr_24(void);
extern void isr_25(void);
extern void isr_26(void);
extern void isr_27(void);
extern void isr_28(void);
extern void isr_29(void);
extern void isr_30(void);
extern void isr_31(void);
extern void isr_32(void);
extern void isr_33(void);
extern void isr_34(void);
extern void isr_35(void);
extern void isr_36(void);
extern void isr_37(void);
extern void isr_38(void);
extern void isr_39(void);
extern void isr_40(void);
extern void isr_41(void);
extern void isr_42(void);
extern void isr_43(void);
extern void isr_44(void);
extern void isr_45(void);
extern void isr_46(void);
extern void isr_47(void);
#endif
