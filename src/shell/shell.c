#include <std/int.h>
#include <std/string.h>

uint32_t copy_num_to_buf(uint64_t num, char *buf) {
    uint8_t digits = 0;
    char nums[25];
    do {
        nums[digits++] = (num%10) + '0';
        num /= 10;
    } while(num);
    uint8_t d_r = digits;
    while(digits --> 0) {
        *buf = nums[digits];
        *buf++;
    }
    return d_r;
}

uint32_t _sprintf(int _a,int _b,int _c,int _d,int _e,int _f,int _g,int _h,int _i,int _j, char *dest, const char fmt[], ...) {
    int chars = 0;
    void **arg;
    char c;
    arg = (void **) &fmt;
    arg++;

    c = *fmt;
    while (c) {
        switch (c) {
            case '%':
                fmt++;
                c = *fmt;
                if (arg) {
                    if (c) {
                        switch(c) {
                            case 's':
                                (void) "Copying a string";
                                char *str = *((char **) arg);
                                uint64_t len = strlen(str);
                                memcpy(dest+chars, str, len);
                                chars += len;
                                break;

                            case 'c':
                                dest[chars++] = *((char *)arg);
                                break;

                            case 'i':
                                (void) "Copying an int.";
                                uint64_t num = *((uint64_t *) arg);
                                chars += copy_num_to_buf(num, dest+chars);
                                break;

                        }
                        arg++;
                    }
                }
                break;
            default:
                dest[chars++] = c;
        }
        if (c) {
            fmt++;
            c = *fmt;
        }
    }
    return chars;
}