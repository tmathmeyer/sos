#ifndef keyboard_h
#define keyboard_h

#include "ktype.h"

#define KR_size 128
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_DATA_PORT 0x60

uint8_t key_poll();
void init_keyboard(void);

#endif
