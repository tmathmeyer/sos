#ifndef interrupts_h
#define interrupts_h

#include "ktype.h"

struct opts {
    uint8_t ist_index   : 3;
    uint8_t reserved    : 5;
    uint8_t int_or_trap : 1;
    uint8_t must_be_one : 3;
    uint8_t must_be_zro : 1;
    uint8_t privelage_l : 2;
    uint8_t present     : 1;
}__attribute__((packed));

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
idt_entry_t create(uint16_t, uint64_t);
uint16_t cs();
struct opts *set_handler(uint8_t loc, uint64_t fn_ptr);
void load_IDT();

#endif
