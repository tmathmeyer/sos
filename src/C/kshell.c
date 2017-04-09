#include "kshell.h"
#include "ktype.h"
#include "libk.h"
#include "kio.h"
#include "mmu.h"
#include "time.h"
#include "pci.h"

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
    } else if (!strncmp(run, "dzfault", 6)) {
        int i=0, j=11;
        kprintf("stack = %6dx\n", &i);
        int k = j/i;
    } else if (!strncmp(run, "pci", 4)) {
        pci_scan(print_pci_devices, -1, NULL);
    } else if (!strncmp(run, "pci_vend 0x", 11)) {
        uint16_t vendid = hex2int(run+11);
        kprintf("full vendor for [%03x] = %03s\n", vendid, pci_vendor_lookup(vendid));
    } else if (!strncmp(run, "frames", 7)) {
        print_frames();
    } else if (!strncmp(run, "pages", 6)) {
        print_pages();
    } else if (!strncmp(run, "fsteal", 7)) {
        kprintf("stole page: %6ex\n", get_next_free_frame());
    } else if (!strncmp(run, "nfp", 4)) {
        kprintf("next free page = %6ex\n", find_page_no_table_creation(4)); 
    } else if (!strncmp(run, "time", 5)) {
        show_time();
    } else if (!strncmp(run, "gpi2 0x", 7)) {
        uint64_t a = hex2int(run+7);
        kprintf("table address = %6ex\n", get_page_index(2, (void *)a)); 
    } else if (!strncmp(run, "palloc", 7)) {
        page_t page = allocate_full_page();
        kprintf("allocated page = %6ex\n", page);
        kprintf("mapped to      = %6ex\n", translate_page(page));
    } else if (!strncmp(run, "kmap 0x", 7)) {
        uint64_t virt = hex2int(run+7);
        char *next = strfnd(run+7, ' ');
        if (*next && !strncmp(next, " 0x", 3)) {
            uint64_t phys = hex2int(next+3);
            kprintf("mapping P%07x to F%07x\n", virt, phys);
            map_page_to_frame(virt, phys);
        } else {
            kprintf("see %05x for usage\n", "help");
        }
    } else if (!strncmp(run, "read 0x", 7)) {
        char *addr = (char *)hex2int(run+7);
        char *next = strfnd(run+7, ' ');
        if (*next && !strncmp(next, " 0x", 3)) {
            uint64_t size = hex2int(next+3);
            for(int j=0;j<size;j++) {
                kprintf("%07c", addr[j]);
            }
            kprintf("\n");
        } else {
            kprintf("see %05x for usage\n", "help");
        }
    } else if (!strncmp(run, "write 0x", 8)) {
        char *addr = (char *)hex2int(run+8);
        kprintf("writing to %07x\n", addr);
        char *next = strfnd(run+7, ' ');
        while (next && *next) {
            *addr++ = *next++;
        }
    } else if (!strncmp(run, "pageinfo 0x", 11)) {
        uint64_t addr = hex2int(run+11);
        char *next = strfnd(run+11, ' ');
        if (next && *next && !strncmp(next, " 0x", 3)) {
            uint64_t size = hex2int(next+3);
            page_t t = containing_address(addr);
            for(size_t i=0; i<size; i++) {
                uint64_t addr = starting_address(t+i);
                kprintf("%05x  -->  %05x\n", addr, translate_address((void *)addr));
            }
        } else {
            kprintf(" see %05x for usage\n", "help");
        }
    } else if (!strncmp(run, "find ", 5)) {
        char *x = (void *)0x1000;
        int i = 0;
        char *ne = run+5;
        while(ne[i]) {
            if (i%4096 == 0) {
                if (translate_address(x) == 0) {
                    goto noaddr;
                }
            }
            (void)"sometimes interrupts break this...";
fucking:
            if (*x == ne[i] && x!=ne) {
                i++;
            } else {
                i = 0;
            }
            x++;
        }
        kprintf("%05x\n", x - stringlen(run+5));
        return;
noaddr:
        if (translate_address(x)) {
            goto fucking;
        }
        kprintf("text not found in ram [%03x]\n", x);
    } else if (!strncmp(run, "help", 5)) {
        kprintf("  page   [0xADDR]:      display page table info for a virtual address\n");
        kprintf("  kmap   [0xA1 0xA2]:   map virtual addr1 to physical addr2\n");
        kprintf("  read   [0xADDR 0xN]:  read N bytes from ADDR\n");
        kprintf("  write  [0xADDR text]: write [text] to ADDR\n");
        kprintf("  help:                 print this information\n");
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
            int i;
            kprintf("\n");
            run_cmd(readline);
            rdl_index = 0;
            for(i=0;i<RDL_SIZE;i++) {
                readline[i] = 0;
            }
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
