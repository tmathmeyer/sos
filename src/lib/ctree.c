#include <ctree.h>
#include <ktype.h>
#include <kio.h>
#include <libk.h>

ctree_t *ctree_get_element(ctree_t *parent, void *search, int (*compare)(void *, void *)) {
	if (parent == NULL) {
		return NULL;
	}
	struct _ctree_l *lstart = parent->branches;
	while(lstart) {
		if (compare(search, lstart->key)) {
			return lstart->branch;
		}
		lstart = lstart->next;
	}
	return NULL;
}

int ctree_string_compare(void *_a, void *_b) {
	char *a = _a;
	char *b = _b;
	return !strncmp(a, b, strlen(a));
}