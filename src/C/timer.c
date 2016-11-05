#include "timer.h"
#include "kio.h"
#include "libk.h"
#include "interrupts.h"

/*
    0x36 = 0011 0110
    BCD  := 0b = 0
    MODE := 101b = 3
    RW   := 11b = 3
    CNTR := 00b = 0
*/
void set_timer_phase(int hz) {
    PIT_8254 cmd;
    cmd.BCD = BCD_16_BIT;
    cmd.mode = MODE_SQR_WAVE;
    cmd.RW = RW_READ | RW_WRITE;
    cmd.CNTR = 0;

    outb(PIT_CMD, 0x36);
    int divisor = 1193180 / hz;
    outb(PIT_CH0, divisor & 0xff);
    outb(PIT_CH0, divisor >> 8);
}

void enable_timer(void) {
    set_timer_phase(100);
    outb(0x21, inb(0x21)&(0xFE));
}
