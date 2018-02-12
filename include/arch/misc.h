/* yeah, this is a _really_ bad file name */

#ifndef misc_h
#define misc_h

#define PAGE 1
#define FRAME 0

#define min(x, y) (((x)>(y))?(y):(x))
#define max(a, b) (((a)>(b))?(a):(b))


void wait(void);
extern void unwind(void **arr, int size);
void dumpstack();


#endif
