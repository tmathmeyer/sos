#ifndef alloc_h
#define alloc_h

void *kmalloc_init(void);
void *kmalloc(uint64_t);
void kfree(void *_ptr);

#endif