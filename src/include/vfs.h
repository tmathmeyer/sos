#ifndef vfs_h
#define vfs_h

#include <fs.h>

void vfs_init(void);
void ls(char *); /* move to a coreutils */
void cat(char *);

void make_fs_entry(char *, int);
void _make_fs_entry(char *, int, fs_t *);
FS_ERROR open_stat(char *, fs_metadata_t *, fs_file_t *);
void mount(void *, fs_t *);

#endif
