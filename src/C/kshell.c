#include "kshell.h"
#include "ktype.h"
#include "libk.h"

unsigned char keyboard_map[128] = {
    0, // null
    0, // escape
    '1', '2', '3', '4',
    '5', '6', '7', '8',
    '9', '0', '-', '=',
    0, // backspace
    0, // tab
    'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i',
    'o', 'p', '[', ']',
    0,	// ENTER
    0,  // L-CONTROL
    'a', 's', 'd', 'f',
    'g', 'h', 'j', 'k',
    'l', ';', '\'', '`',
    0, // L-SHIFT
    '\\', 'z', 'x', 'c',
    'v', 'b', 'n', 'm',
    ',', '.', '/',
    0, // R-SHIFT
    '*',
    0, // L-ALT
    ' ',
    0, // CAPS-LOCK
       // FUNCTION KEYS
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0,
    0,	// NUM-LOCK
    0,	// SCROLL-LOCK
    0,	// HOME-KEY
    0,	// UP-ARROW
    0,	// PAGE-UP
    '-',
    0,	// LEFT-ARROW
    0,   // UNKNOWN?!?!?
    0,	// RIGHT-ARROW
    '+',
    0,  // END
    0,	// DOWN-ARROW
    0,	// PAGE-DOWN
    0,	// INSERT
    0,	// DELETE
    0, 0, 0, // UNKNOWN
    0,	// F11
    0,	// F12
    0,	// UNKNOWN
};

#define KB_NULL    0
#define KB_ESC     1
#define KB_BACK    14
#define KB_TAB     15
#define KB_ENTER   28
#define KB_LCTRL   29
#define KB_LSHIFT  42
#define KB_RSHIFT  54

bool caps_mode = false;
bool shift_mode = false;
bool alt_mode = false;
bool ctrl_mode = false;
bool super_mode = false;

void run_cmd() {
    
}

#define CASEMACRO(K) case K: \
    if (up) kprintf("[%03s]\n", #K); break;

void kshell(unsigned char key) {
    bool up = false;
    if (key > 128) {
        up = true;
        key -= 128;
    }
    switch(key) {
        CASEMACRO(KB_ESC);
        CASEMACRO(KB_BACK);
        CASEMACRO(KB_TAB);
        CASEMACRO(KB_ENTER);
        CASEMACRO(KB_LCTRL);

        case KB_LSHIFT:
        case KB_RSHIFT:
            if (up) {
                shift_mode = 0;
            } else {
                shift_mode = 1;
            }
            break;
        default:
        {
            unsigned char result = keyboard_map[key];
            if (up) {
                if (result) {
                    kprintf("%03c", result);
                } else {
                    unsigned int x = key;
                    kprintf("\nkey[%03i]\n", x);
                }
            }
            break;
        }
    }
}
