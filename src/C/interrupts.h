#ifndef interrupts_h
#define interrupts_h

#include "ktype.h"

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
uint16_t cs();
struct opts *set_handler(uint8_t loc, uint64_t fn_ptr);
void load_IDT();

#endif