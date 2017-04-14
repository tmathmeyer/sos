#include "libk.h"
#include "registers.h"

inline uint16_t cs(void) {
    uint16_t val;
    asm volatile ( "mov %%cs, %0" : "=r"(val) );
    return val;
}
