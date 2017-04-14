#include "libk.h"

#define outb_p(value,port) \
__asm__ ("outb %%al,%%dx\n" \
		"\tjmp 1f\n" \
		"1:\tjmp 1f\n" \
		"1:"::"a" (value),"d" (port))

#define inb_p(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al\n" \
	"\tjmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:":"=a" (_v):"d" (port)); \
_v; \
})

#define CMOS_READ(addr) ({ \
    outb_p(0x80|addr,0x70); \
    inb_p(0x71); \
})

void show_time() {
    kprintf("%03i/%03i/%03i %03i:%03i:%03i\n",
            CMOS_READ(8), CMOS_READ(7), CMOS_READ(9),
            CMOS_READ(4), CMOS_READ(2), CMOS_READ(0));
}
