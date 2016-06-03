#include "ktype.h"
#include "interrupts.h"
#include "cpuio.h"

void intel_8259_set_irq_mask(uint16_t mask) {
    outb(I8259_PORT_MASTER_DATA, mask & 0xFF);
    outb(I8259_PORT_SLAVE_DATA, (mask >> 8) & 0xFF);
}

uint16_t intel_8259_get_irq_mask() {
    uint8_t mask_lo = inb(I8259_PORT_MASTER_DATA);
    uint8_t mask_hi = inb(I8259_PORT_SLAVE_DATA);
    return (mask_hi << 8) | mask_lo;
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
