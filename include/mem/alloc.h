#ifndef alloc_h
#define alloc_h

void *kmalloc_init(void);
void *kmalloc(uint64_t);
void *kcmalloc(uint64_t, uint64_t);
void kfree(void *_ptr);

void *memset(void *p, int b, size_t n);
void *memcpy(void *dest, void *src, size_t bytes);

#endif