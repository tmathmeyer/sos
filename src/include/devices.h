#ifndef devices_h
#define devices_h

#include "shittyfs.h"

void dev_init();
fs_t *get_device(char *);
void put_device(char *, fs_t);
void list_devices();

void each_device(void (*each)(fs_t *, char *));

#endif