#ifndef devfs_h
#define devfs_h

#include <ata.h>
#include <libk.h>

void devfs_put_device(char *, struct ata_device *);
void devfs_init(void);
struct ata_device *get_ata_by_dev_dir(char *);

#endif
