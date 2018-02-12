#include <kernel/kernel_info.h>
#include <fs/virtual_filesystem.h>
#include <shell/shell.h>
#include <std/string.h>

void kernel_info_init() {
    mkdir("/kernel");

    int fd = open("/kernel/build_info", CREATE_ON_OPEN);
    if (!fd) {
    	kprintf("failed to open kernel info file\n");
    	return;
    }
    char *build_info = "Kernel Compilation Info:\n"
                       "  build date: " __DATE__ "\n"
                       "  build time: " __TIME__ "\n"
                       "  gcc compiler version: " __VERSION__ "\n"
                       "  build version: " SOS_VERSION "\n"
    int wb = write(fd, build_info, strlen(build_info));
    if (wb < 10) {
    	kprintf("(%06i) %05s\n", wb, "failed to write kernel info to file");
    }

    close(fd);
}