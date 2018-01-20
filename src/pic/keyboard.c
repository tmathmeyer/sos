#include "interrupts.h"
#include "keyboard.h"
#include "ktype.h"
#include "kio.h"
#include "libk.h"


#define RINGSIZE 32
uint64_t readdex = 0;
uint64_t writedex = 0;
uint8_t keyring[RINGSIZE] = {0};

/* the keyboard interrupt handler --- gatta go fast! */
void keyboard(struct interrupt_frame *frame) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        uint8_t code = inb(KEYBOARD_DATA_PORT);
        uint64_t nwrite = (writedex + 1) % RINGSIZE;
        keyring[writedex] = code;
        if (nwrite != readdex) {
            writedex = nwrite;
        }
    }
}

uint8_t key_poll() {
    if(readdex == writedex) {
        return 0;
    }
    uint8_t cur = keyring[readdex];
    readdex = (readdex + 1) % RINGSIZE;
    return cur;
}

void init_keyboard(void) {
    outb(PIC1_DATA, 0xfd);
    outb(PIC2_DATA, 0xff);
    set_interrupt_handler(0x21, keyboard);
}
