#ifndef interrupts_h
#define interrupts_h

#include "ktype.h"

/*
#define I8259_IRQ_CASCADE 2
#define I8259_MASK_CASCADE ((uint16_t)(1 << I8259_IRQ_CASCADE))
#define I8259_MASK_ALL (0xFFFF & ~I8259_MASK_CASCADE)

#define I8259_PORT_MASTER_COMMAND 0x20
#define I8259_PORT_MASTER_DATA 0x21
#define I8259_PORT_SLAVE_COMMAND 0xA0
#define I8259_PORT_SLAVE_DATA 0xA1

#define I8259_ICW1_ICW4  0x01       // ICW4 (not) needed
#define I8259_ICW1_SINGLE 0x02      // Single (cascade) mode
#define I8259_ICW1_INTERVAL4  0x04  // Call address interval 4 (8)
#define I8259_ICW1_LEVEL  0x08      // Level triggered (edge) mode
#define I8259_ICW1_INIT 0x10        // Initialization - required!

#define I8259_ICW4_8086 0x01        // 8086/88 (MCS-80/85) mode
#define I8259_ICW4_AUTO 0x02        // Auto (normal) EOI
#define I8259_ICW4_BUF_SLAVE 0x08   // Buffered mode/slave
#define I8259_ICW4_BUF_MASTER 0x0C  // Buffered mode/master
#define I8259_ICW4_SFNM 0x10        // Special fully nested (not)

void intel_8259_set_idt_start(int);
void load_idt(void);


typedef struct descriptor_table {
    uint16_t limit;
    uint64_t offset;
} descriptor_table_t;
*/


struct opts {
    uint8_t ist_index   : 3;
    uint8_t reserved    : 5;
    uint8_t int_or_trap : 1;
    uint8_t must_be_one : 3;
    uint8_t must_be_zro : 1;
    uint8_t privelage_l : 2;
    uint8_t present     : 1;
};

typedef struct idt_entry {
	uint16_t ptr_low;
	uint16_t selector;
    struct opts opts;
    uint16_t ptr_mid;
    uint32_t ptr_high;
    uint32_t _zero;
}__attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t max_byte;
    uint64_t virt_start;
}__attribute__((packed)) descriptor_table_t;



idt_entry_t create_empty();
idt_entry_t create(uint16_t, size_t);
uint16_t cs();
struct opts *set_handler(uint8_t loc, size_t fn_ptr);
void load_IDT();

#endif
