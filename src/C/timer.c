#include "timer.h"
#include "kio.h"
#include "libk.h"
#include "interrupts.h"

int i = 0;
int j = 0;
void timer(struct interrupt_frame *frame) {
    i++;
    if (!(i&0x7)) {
        j++;
        j%=10;
    }
}

void set_timer_phase(int hz) {
    PIT_8254 cmd;
    cmd.BCD = BCD_16_BIT;
    cmd.mode = MODE_SQR_WAVE;
    cmd.RW = RW_READ | RW_WRITE;
    cmd.CNTR = 0;

    outb(PIT_CMD, cmd.cmd);
    int divisor = 1193180 / hz;
    outb(PIT_CH0, divisor & 0xff);
    outb(PIT_CH0, divisor >> 8);
}

void init_timer() {
    set_timer_phase(20);

    set_interrupt_handler(0x20, &timer);
    outb(PIC1_DATA, inb(PIC1_DATA)&~0x01);
}
