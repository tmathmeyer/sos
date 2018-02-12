#ifndef lock_h
#define lock_h

#include <std/int.h>

typedef int lock_t;
typedef int ref_t;

bool try_lock(int *p);
void spin_lock(int *p);
void spin_unlock(int volatile *p);

ref_t ref_count_inc(ref_t *p);
ref_t ref_count_dec(ref_t *p);

#endif
