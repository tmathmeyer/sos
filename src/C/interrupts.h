#ifndef interrupts_h
#define interrupts_h

#include "ktype.h"

#define I8259_IRQ_CASCADE 2
#define I8259_MASK_CASCADE ((uint16_t)(1 << I8259_IRQ_CASCADE))
#define I8259_MASK_ALL (0xFFFF & ~I8259_MASK_CASCADE)

#define I8259_PORT_MASTER_COMMAND 0x20
#define I8259_PORT_MASTER_DATA 0x21
#define I8259_PORT_SLAVE_COMMAND 0xA0
#define I8259_PORT_SLAVE_DATA 0xA1

#define I8259_ICW1_ICW4  0x01       /* ICW4 (not) needed */
#define I8259_ICW1_SINGLE 0x02      /* Single (cascade) mode */
#define I8259_ICW1_INTERVAL4  0x04  /* Call address interval 4 (8) */
#define I8259_ICW1_LEVEL  0x08      /* Level triggered (edge) mode */
#define I8259_ICW1_INIT 0x10        /* Initialization - required! */

#define I8259_ICW4_8086 0x01        /* 8086/88 (MCS-80/85) mode */
#define I8259_ICW4_AUTO 0x02        /* Auto (normal) EOI */
#define I8259_ICW4_BUF_SLAVE 0x08   /* Buffered mode/slave */
#define I8259_ICW4_BUF_MASTER 0x0C  /* Buffered mode/master */
#define I8259_ICW4_SFNM 0x10        /* Special fully nested (not) */

void intel_8259_set_idt_start(int index);
void intel_8259_enable_irq(int irqno);
void intel_8259_set_irq_mask(uint16_t mask);
uint16_t intel_8259_get_irq_mask();

typedef struct {
    uintptr_t rip;
    uintptr_t code_selector;
    uint64_t rflags;
    uintptr_t rsp;
    uintptr_t stack_selector;
} iretq_state_t;

typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} register_state_t;

typedef enum {
    CPU_EXC0,
    CPU_EXC1,
    CPU_EXC2,
    CPU_EXC3,
    CPU_EXC4,
    CPU_EXC5,
    CPU_EXC6,
    CPU_EXC7,
    CPU_EXC8,
    CPU_EXC9,
    CPU_EXC10,
    CPU_EXC11,
    CPU_EXC12,
    CPU_EXC13,
    CPU_EXC14,
    CPU_EXC15,
    CPU_EXC16,
    CPU_EXC17,
    CPU_EXC18,
    CPU_EXC19,
    CPU_EXC20,
    CPU_EXC21,
    CPU_EXC22,
    CPU_EXC23,
    CPU_EXC24,
    CPU_EXC25,
    CPU_EXC26,
    CPU_EXC27,
    CPU_EXC28,
    CPU_EXC29,
    CPU_EXC30,
    CPU_EXC31,
} cpu_exception_e;

typedef enum {
    IRQ0,
    IRQ1,
    IRQ2,
    IRQ3,
    IRQ4,
    IRQ5,
    IRQ6,
    IRQ7,
    IRQ8,
    IRQ9,
    IRQ10,
    IRQ11,
    IRQ12,
    IRQ13,
    IRQ14,
    IRQ15,
    MAX_IRQ,

} irq_e;

typedef struct {
    register_state_t registers;
    iretq_state_t iretq;
} system_state_t;

#endif
