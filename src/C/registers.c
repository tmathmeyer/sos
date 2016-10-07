#include "libk.h"
#include "registers.h"

inline uint16_t cs(void) {
    uint16_t val;
    asm volatile ( "mov %%cs, %0" : "=r"(val) );
    return val;
}

inline uint64_t rip(void) {
    uint64_t val;
    extern uint64_t __rax();
    val = __rax();
    return val;
}
