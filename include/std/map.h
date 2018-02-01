#ifndef map_h
#define map_h

#include <std/int.h>

#define MAP_MISSING -3  /* No such element */
#define MAP_FULL -2 	/* Hashmap is full */
#define MAP_OMEM -1 	/* Out of Memory */
#define MAP_OK 0 	/* OK */

// You don't get to know what a map_t is!
typedef void *map_t;

// For iterating through hashmaps as a simple list
typedef struct {
	char *key;
	void *data;
} hashmap_iterator_element;

typedef struct {
	int position;
	int maximum;
	hashmap_iterator_element *data;
} hashmap_iterator;

int hashmap_put(map_t in, char *key, void *value);

int hashmap_get(map_t in, char *key, void **arg);

int hashmap_remove(map_t in, char *key);

void hashmap_free(map_t in);

int hashmap_size(map_t in);

map_t hashmap_new();

hashmap_iterator *map_iterator(map_t in);
int hashmap_iterator_has_next(hashmap_iterator *itr);
void hashmap_iterator_done(hashmap_iterator *itr);
hashmap_iterator_element *hashmap_iterator_next(hashmap_iterator *itr);

#define hashmap_iterate(m, k, v) \
	for(void *___i=map_iterator(m),*___m=NULL; \
		hashmap_iterator_has_next(___i)?(___m=hashmap_iterator_next(___i), \
			v=((hashmap_iterator_element *)___m)->data, \
			k=((hashmap_iterator_element *)___m)->key):0;)

#endif