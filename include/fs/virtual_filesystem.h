#ifndef root_h
#define root_h

#include <fs/fs.h>

void root_init();
int mount(char *path, filesystem_t *fs);
int open(char *, uint16_t);
int close(int);
int read(int, void *, uint64_t);
int write(int, void *, uint64_t);
int delete(char *path);
void stat(int);
int mkdir(char *);
int scan_dir(int, char **);
F_type node_type(char *path);
void tree(char *path);
char *basename(char *);
char *dirname(char *);
char *err2msg(F_err);
void show();


#endif