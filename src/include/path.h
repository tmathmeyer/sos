#ifndef path_h
#define path_h

#include <ktype.h>

typedef struct {
    uint16_t section;
    uint16_t total_sections;
    char *__inside_string;
    char *string;
} path_t;

path_t *_allocate_path(char *);
void __path_next(path_t *);

#define path_for_each(path, seg) \
    for(seg=path->__inside_string; \
        path->section<path->total_sections; \
        __path_next(path),seg=path->__inside_string)
    
#endif
