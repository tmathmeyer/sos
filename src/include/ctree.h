#ifndef ctree_h
#define ctree_h

typedef
struct _ctree_s {
	void *data;
	struct _ctree_l *branches; 
} ctree_t;

struct _ctree_l {
	void *key;
	ctree_t *branch;
	struct _ctree_l *next;
};

ctree_t *ctree_get_element(ctree_t *, void *, int (*)(void *, void *));
int ctree_string_compare(void *a, void *b);

#endif