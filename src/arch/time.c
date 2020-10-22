#include <shell/shell.h>
#include <arch/time.h>
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
    outb_p(addr,0x70); \
    inb_p(0x71); \
})

void show_time() {
	// referenced from : https://wiki.osdev.org/CMOS#RTC_Update_In_Progress
	// reading values from associated CMOS register number containing respective data
	
	uint8_t hour, minute, second, day, month, year;
	
	hour = CMOS_READ(0x04);
	minute = CMOS_READ(0x02);
	second = CMOS_READ(0x00);

	day = CMOS_READ(0x07);
	month = CMOS_READ(0x08);
	year = CMOS_READ(0x09);
	

	// Converting BCD to binary values
	second = (second & 0x0F) + ((second / 16) * 10);
	minute = (minute & 0x0F) + ((minute / 16) * 10);
	hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);

	day = (day & 0x0F) + ((day / 16) * 10);
	month = (month & 0x0F) + ((month / 16) * 10);
	year = (year & 0x0F) + ((year / 16) * 10);
	
	// Converting 12 hour clock to 24 hour clock 
	if (!(cMOS_READ(0x0B) & 0x02) && (hour & 0x80)) {
		hour = ((hour & 0x7F) + 12) % 24;
	}   
    
	kprintf("%03i/%03i/'%03i %03i:%03i:%03i UTC\n", day, month, year, hour, minute, second);
}
