#include <arch/lock.h>

bool try_lock(lock_t *p) {
    if (__sync_bool_compare_and_swap(p, 0, 1)) {
        return true;
    }
    return false;
}

void spin_lock(lock_t *p) {
    while(!try_lock(p)) {
        __asm__("hlt");
    }
}

void spin_unlock(lock_t volatile *p) {
    asm volatile ("");
    *p = 0;
}

ref_t ref_count_inc(ref_t *p) {
	return __sync_fetch_and_add(p, 1);
}

ref_t ref_count_dec(ref_t *p) {
	return __sync_sub_and_fetch(p, 1);
}