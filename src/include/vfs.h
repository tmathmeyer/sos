#ifndef vfs_h
#define vfs_h

#include <fs.h>

void vfs_init(void);
void mount_fs_by_path(fs_t *, char *);

void ls(char *);

#endif