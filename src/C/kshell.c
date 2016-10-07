#include "kshell.h"
#include "ktype.h"
#include "libk.h"
#include "kio.h"
#include "paging.h"

uint8_t keymap[][128] = {
    {0},
    {0},
    {'1', 1, '!'},
    {'2', 1, '@'},
    {'3', 1, '#'},
    {'4', 1, '$'},
    {'5', 1, '%'},
    {'6', 1, '^'},
    {'7', 1, '&'},
    {'8', 1, '*'},
    {'9', 1, '('},
    {'0', 1, ')'},
    {'-', 1, '_'},
    {'=', 1, '+'},
    {0},
    {0},
    {'q', 2, 'Q'},
    {'w', 2, 'W'},
    {'e', 2, 'E'},
    {'r', 2, 'R'},
    {'t', 2, 'T'},
    {'y', 2, 'Y'},
    {'u', 2, 'U'},
    {'i', 2, 'I'},
    {'o', 2, 'O'},
    {'p', 2, 'P'},
    {'[', 1, '{'},
    {']', 1, '}'},
    {0},
    {0},
    {'a', 2, 'A'},
    {'s', 2, 'S'},
    {'d', 2, 'D'},
    {'f', 2, 'F'},
    {'g', 2, 'G'},
    {'h', 2, 'H'},
    {'j', 2, 'J'},
    {'k', 2, 'K'},
    {'l', 2, 'L'},
    {';', 1, ':'},
    {'\'', 1, '"'},
    {'`', 1, '~'},
    {0},
    {'\\', 1, '|'},
    {'z', 2, 'Z'},
    {'x', 2, 'X'},
    {'c', 2, 'C'},
    {'v', 2, 'V'},
    {'b', 2, 'B'},
    {'n', 2, 'N'},
    {'m', 2, 'M'},
    {',', 1, '<'},
    {'.', 1, '>'},
    {'/', 1, '?'},
    {0},
    {0},
    {0},
    {' ', 0},
    {0}, // caps lock
         // function 1-10 keys
    {0}, {0}, {0}, {0}, {0},
    {0}, {0}, {0}, {0}, {0},
    {0}, // num lock
    {0}, // scroll lock
    {0}, // home
    {0}, // up arrow
    {0}, // page up
    {'-', 1, '_'},
    {0}, // left arrow
    {0}, // unknown?
    {0}, // right arrow
    {'=', 1, '='},
    {0}, // end
    {0}, // down arrow
    {0}, // page down
    {0}, // insert
    {0}, // delete
    {0}, // unknown?
    {0}, // unknown?
    {0}, // unknown?
    {0}, // f11
    {0}, // f12
    {0}, // unknown
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

void run_cmd(char *run) {
    if (!(strncmp(run, "page 0x", 7))) {
        uint64_t num = hex2int(run+7);
        show_page_table_layout_for_address(num);
    } else if (!strncmp(run, "stack", 6)) {
        int i;
        int j;
        kprintf("The stack address is %03x\n", &i);
        kprintf("The stack address is %03x\n", &j);
    } else if (!strncmp(run, "frames 0x", 9)) {
        uint64_t num = hex2int(run+9);
        print_frame_alloc_table_list_entry(num);
    } else if (!strncmp(run, "kalloc", 7)) {
        page_t p = allocate_page();
        kprintf("your ident mapped page is %03x\n", starting_address(p));
    } else if (!strncmp(run, "kmap 0x", 7)) {
        uint64_t virt = hex2int(run+7);
        char *next = strfnd(run+7, ' ');
        if (*next && !strncmp(next, " 0x", 3)) {
            uint64_t phys = hex2int(next+3);
            kprintf("mapping P%07x to F%07x\n", containing_address(virt), containing_address(phys));
            map_page_to_frame(containing_address(virt), containing_address(phys), 0, (void *)0);
        } else {
            kprintf("see usage for details\n");
        }
    } else if (!strncmp(run, "read 0x", 7)) {
        char *addr = (char *)hex2int(run+7);
        int i = 7;
        for(;run[i];i++) {
            if (run[i] == ' ') {
                i++;
                break;
            }
        }
        if (!strncmp(run+i, "0x", 2)) {
            uint64_t size = hex2int(run+i+2);
            for(int j=0;j<size;j++) {
                kprintf("%07c", addr[j]);
            }
            kprintf("\n");
        }
    } else if (!strncmp(run, "write 0x", 8)) {
        char *addr = (char *)hex2int(run+8);
        kprintf("writing to %07x\n", addr);
        int i = 8;
        for(;run[i];i++) {
            if (run[i] == ' ') {
                i++;
                break;
            }
        }
        for(int j=0;run[i];i++,j++) {
            addr[j] = run[i];
        }
    } else if (!strncmp(run, "help", 5)) {
        kprintf("SOS commands:\n");
        kprintf("  page [0xdeadbeef]:   display page table info for a virtual address\n");
        kprintf("  stack:               get the stack address\n");
        kprintf("  frames [0xdeadbeef]: print information for the frame allocation table\n");
        kprintf("  help:                print this information\n");
    } else {
        kprintf("INVALID COMMAND\n");
    }
}

#define CASEMACRO(K) case K: \
    if (up) kprintf("[%03s]\n", #K); break;


static inline uint8_t get_char(unsigned char key) {
    uint8_t *map = keymap[key];
    uint8_t def = *map;
    if (!def) { 
        return 0;
    }
    uint8_t affection = map[1];
    if (affection == 0) {
        return def;
    }
    if (affection == 1 && shift_mode) {
        return map[2];
    }
    if (affection == 2 && shift_mode && !caps_mode) {
        return map[2];
    }
    if (affection == 2 && !shift_mode && caps_mode) {
        return map[2];
    }
    return def;
}

char readline[RDL_SIZE] = {0};
uint8_t rdl_index = 0;

void kshell(unsigned char key) {
    bool up = false;
    if (key > 128) {
        up = true;
        key -= 128;
    }
    switch(key) {
        CASEMACRO(KB_ESC);
        CASEMACRO(KB_TAB);
        CASEMACRO(KB_LCTRL);

        case KB_ENTER:
        if (up) {
            kprintf("\n");
            run_cmd(readline);
            rdl_index = 0;
            memset(readline, 0, RDL_SIZE);
            kprintf("%04s", "SOS$ ");
        }
        break;

        case KB_BACK:
        if (up && rdl_index) {
            backspace();
            readline[--rdl_index] = 0;
        }
        break;
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
            if (up) {
                if (rdl_index < RDL_SIZE) {
                    unsigned char result = get_char(key);
                    if (result) {
                        kprintf("%03c", result);
                        readline[rdl_index++] = result;
                    }
                }
            }
            break;
        }
    }
}
