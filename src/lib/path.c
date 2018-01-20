#include <path.h>
#include <alloc.h>

path_t *_allocate_path(char *fromstring) {
	path_t *result = kmalloc(sizeof(path_t));
	result->section = 0;
	result->string = fromstring;
	result->total_sections = 0;
	result->__inside_string = fromstring;
	while(*fromstring) {
		if (*fromstring == '/') {
			*fromstring = 0;
			result->total_sections++;
		}
		fromstring++;
	}
	return result;
}

void __path_next(path_t *path) {
	while(*(path->__inside_string)) {
		path->__inside_string++;
	}
	path->__inside_string++;
	path->section++;
}