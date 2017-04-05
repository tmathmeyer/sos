#include "interrupts.h"
#include "keyboard.h"
#include "ktype.h"
#include "kio.h"
#include "libk.h"
#include "lock.h"

lock_t readlock = 0;
lock_t writelock = 0;
uint8_t keycode = 0;

/* the keyboard interrupt handler --- gatta go fast! */
void keyboard(struct interrupt_frame *frame) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        uint8_t code = inb(KEYBOARD_DATA_PORT);
        if (try_lock(&readlock)) {
            keycode = code;
            spin_unlock(&readlock);
            spin_unlock(&writelock);
        }
    }
}

uint8_t key_poll() {
    spin_lock(&writelock);
    spin_lock(&readlock);
    uint8_t cur = keycode;
    spin_unlock(&readlock);
    return cur;
}

void init_keyboard(void) {
    outb(PIC1_DATA, 0xfd);
    outb(PIC2_DATA, 0xff);
    set_interrupt_handler(0x21, keyboard);
    spin_lock(&writelock);
}
