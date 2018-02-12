#ifndef timer_h
#define timer_h

#include <std/int.h>

#define PIT_CH0 0x40
#define PIT_CH1 0x41
#define PIT_CH2 0x42
#define PIT_CMD 0x43

typedef
union {
    uint8_t cmd;
    struct {
        uint8_t BCD  : 1;
        uint8_t mode : 3;
        uint8_t RW   : 2;
        uint8_t CNTR : 2;
    };
} PIT_8254;

#define BCD_16_BIT 0
#define BCD_4x_DEC 1

#define MODE_IO_TERM_CT 0
#define MODE_HW_RETRIG  1
#define MODE_RATE_GEN   2
#define MODE_SQR_WAVE   3
#define MODE_SFT_STROBE 4
#define MODE_HWD_STROBE 5

#define RW_READ 1
#define RW_WRITE 2

void init_timer();

#endif
