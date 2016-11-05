#include "scheduler.h"
#include "kio.h"

char i = 'a';
void reschedule(void) {
    force_char(i, 0x5, 75, 2);
    i++;
    if (i == 'z') {
        i = 'a';
    }
}
