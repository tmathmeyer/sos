#include <ktype.h>
#include <path.h>
#include <libk.h>
#include <alloc.h>

inline uint64_t count(char *in, char c) {
	uint64_t res = 0;
	while(*in) {
		if (*in == c) {
			res++;
		}
		in++;
	}
	return res;
}

inline char *path_repair(path_t path) {
    for(int i=0; i<path.count; i++) {
        path.ptrs[i][-1] = '/';
    }
    return NULL;
}

inline path_t path_from_string(char *path) {
    uint64_t parts = count(path, '/');
	uint64_t len = strlen(path);

    path_t result;
    result.count = parts;
    result.has_trailing = (path[len-1]=='/')?1:0;
    result.original=path;
    for(int i=0; i<100; i++) result.ptrs[i] = 0;

	int i = 0;
	do {
		if (*path == '/') {
			*path = 0;
			result.ptrs[i++] = path+1;
		}
		path++;
	} while(i != parts);
	return result;
}

inline char *path_join(char *a, char *b) {
    int a_len = strlen(a);
    int b_len = strlen(b);
    if (a[a_len-1] == '/') {
        a_len -= 1;
    }
    if (b[0] == '/') {
        b_len -= 1;
        b++;
    }
    char *result = kmalloc(a_len+b_len+2);
    memcpy(result, a, a_len);
    result[a_len] = '/';
    memcpy(result+a_len+1, b, b_len);
    result[a_len+b_len+1] = 0;
    return result;
}

