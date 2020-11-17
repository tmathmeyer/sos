#include <arch/io.h>
#include <pic/timer.h>
#include <pic/interrupts.h>

unsigned long ticks; // The amount of ticks that have passed
int hz; // Self-Explanitory

// Timer callback
void timer(struct interrupt_frame * frame) {
  ticks++;
  return;
}

// Returns how many seconds the computer wa
double get_uptime() {
  return ticks * (1 / hz);
}

void set_timer_phase() {
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
  hz = 20;
  secs = 0;
  ticks = 0;
  set_timer_phase();

  set_interrupt_handler(0x20, & timer);
  outb(PIC1_DATA, inb(PIC1_DATA) & ~0x01);
}
