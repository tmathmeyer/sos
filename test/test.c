#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint64_t size;
    char *__underlying__[0];
} split_t;

char **split(char *, char);
void split_free(char **);

#define splitlen(strs) \
    (((split_t *)(strs))[-1].size)


char **split(char *str, char sep) {
    uint64_t size = 1;
    char *underlying = strdup(str);
    while(*str) {
        if (*str == sep) {
            size++;
        }
        str++;
    }

    split_t *s = malloc(sizeof(split_t) + sizeof(char *)*size);
    s->size = size;
    for (int i=0; i<s->size;i++) {
        s->__underlying__[i] = underlying;
        if (i+1 < s->size) {
            while(*underlying && *underlying != sep) {
                underlying++;
            }
            if (*underlying == sep) {
                *underlying = 0;
                underlying++;
            }
        }
    }

    return s->__underlying__;
}

int main() {
    char **spl = split("/this/is/a/directory", '/');

    printf("%x\n", spl);

    printf("%i\n", splitlen(spl));

    for(int i=1; i<splitlen(spl); i++) {
        printf("%s\n", spl[i]);
    }
}