#include <arch/misc.h>
#include <shell/shell.h>

void wait(void) {
    for(int i=0;i<10000000;i++);
}

void dumpstack() {
	void *arr[5];
    unwind(arr, 5);
    for(int i=0; i<5; i++) {
        kprintf("#%03i: %04x\n", i, arr[i]);
    }
}