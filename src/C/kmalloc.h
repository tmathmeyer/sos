
#ifndef kmalloc_h
#define kmalloc_h

#define block_size(_aht) ((_aht)->packed & 0xffffffffffffffe0)
#define block_alloc(_aht) ((_aht)->packed & 0x01)
#define set_block_size(_aht, size) ((_aht)->packed = (size&0xffffffffffffffe0)|block_alloc(_aht))

typedef struct {
    uint64_t packed;
} alloc_hdr_t;

typedef struct avl_s {
    alloc_hdr_t *ref;
    struct avl_s *smaller;
    struct avl_s *larger;
} avl_t;

typedef 
struct empty_avl_stack {
    void *address;
    struct empty_avl_stack *next;
} avl_stack_t;


void mem_init();
void avl_print();
void *kmalloc(uint64_t);
void kfree(void *);
void print_mem();
uint64_t size_round(uint64_t);
alloc_hdr_t *get_best_fit(uint64_t);
void *allocate_in_block(alloc_hdr_t *, uint64_t);
void print_mem();

#endif
