#include <shell/shell.h>
#include <std/int.h>

#include <mmu/mmu.h>
#include <pci/pci.h>
#include <arch/time.h>
#include <fs/virtual_filesystem.h>
#include<mem/alloc.h>

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

/*max history data limit*/
#define MAX_CMD 100

bool caps_mode = false;
bool shift_mode = false;
bool alt_mode = false;
bool ctrl_mode = false;
bool super_mode = false;


/* dynamic data strecture*/
struct Dynamic_List {
    char data[RDL_SIZE];
    struct Dynamic_List * next;
};


/*head of dynamic list*/
struct Dynamic_List *head=NULL;


void Add_data(char* data,uint8_t size){
    /*if head node is empty*/
    if(head==NULL){
        head=(struct Dynamic_List *)kmalloc(sizeof(struct Dynamic_List));
        memcpy(head->data,data,size);
        head->next=NULL;
        return;
    }
    /*if head node is not empty*/
    struct Dynamic_List *node=NULL, *current=head;
    uint8_t i=0;

    /*traverse to the end of dynamic list*/
    while (current!=NULL && current->next!=NULL){
        current=current->next;
        i++;
    }

    /*check boundry conditions*/
    if(i+1>MAX_CMD)head=head->next;
    
    /*allocate the memory*/
    node=(struct Dynamic_List *)kmalloc(sizeof(struct Dynamic_List));
    
    /*add data to new node*/
    memcpy(node->data,data,size);
    node->next=NULL;

    /*assign new node at the end of list*/
    if(current==NULL)current=node;
    else current->next=node;
}

void print_history(){
    /*pointer for travel through list*/
    struct Dynamic_List *node;
    node=head;
    uint8_t i=0;
    /*print data until the end of list*/
    while((node->next)!=NULL){
        kprintf("%07i %05s\n",i+1,node->data);
        node=node->next;
        i++;
    }
}

void run_cmd(char *run) {
    if (!(strncmp(run, "page 0x", 7))) {
        show_page_table_layout_for_address(hex2int(run+7));
    } else if (!strncmp(run, "pci", 4)) {
        pci_scan(print_pci_devices, -1, NULL);
    } else if (!strncmp(run, "pci_vend 0x", 11)) {
        uint16_t vendid = hex2int(run+11);
        kprintf("full vendor for [%03x] = %03s\n", vendid, pci_vendor_lookup(vendid));
    } else if (!strncmp(run, "frames", 7)) {
        print_frames();
    } else if (!strncmp(run, "pages", 6)) {
        print_pages();
    } else if (!strncmp(run, "time", 5)) {
        show_time();
    } else if (!strncmp(run, "cat ", 4)) {
        int f = open(run + 4, 0);
        if (!f) {
            kprintf("file not found\n");
        } else {
            char buf[17] = {0};
            int in = 0;
            while(in = read(f, buf, 16)) {
                kprintf("%04s", buf);
            }
            kprintf("\n");
        }
    } else if (!strncmp(run, "shutdown", 8)){
        outs(0x604, 0x2000);
    } else if (!strncmp(run, "tree ", 5)) {
        tree(run + 5);
    }
    else if (!strncmp(run, "history", 7)){
        print_history();

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
            Add_data(readline,rdl_index);
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
