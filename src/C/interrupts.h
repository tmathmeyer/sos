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

#endif
