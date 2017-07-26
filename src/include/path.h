#ifndef path_h
#define path_h

#define path_iterate(path, part, countdown) \
    path_t __path = path_from_string(path); \
    size_t __i; \
    countdown = __path.count; \
    for(part=__path.ptrs[0], __i=0; \
        countdown--, \
        part=((__i==__path.count-__path.has_trailing)? \
            (path_repair(__path)): \
            (__path.ptrs[__i])), \
            __i<__path.count-__path.has_trailing; \
        __i++)

#define goto_safe(x) path_repair(__path); goto x;

typedef struct {
    int count;
    char *original;
    char *ptrs[100];
    int has_trailing;
} path_t;

uint64_t count(char *, char);
char *path_repair(path_t);
path_t path_from_string(char *);
char *path_join(char *a, char *b);

#endif
