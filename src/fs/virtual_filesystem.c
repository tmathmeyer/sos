#include <fs/virtual_filesystem.h>
#include <std/map.h>
#include <std/int.h>
#include <std/string.h>
#include <fs/fs.h>
#include <mem/alloc.h>

#include <shell/shell.h>

#define MAX_FILES 256

map_t mounts = NULL;
F files[MAX_FILES];

void root_init() {
    mounts = hashmap_new();
    for (int i=0; i<MAX_FILES; i++) {
        files[i].__open__ = 0;
    }
}

F *next_available_file(int *id) {
    *id = 0;
    for(int i=1; i<MAX_FILES; i++) {
        if (files[i].__open__ == 0) {
            files[i].__open__ = 1;
            *id = i;
            return &files[i];
        }
    }
    return NULL;
}


int mount(char *path, filesystem_t *fs) {
    return hashmap_put(mounts, path, fs);
}

int open(char *path, uint16_t FLAGS) {
    if (path[0] != '/') {
        return 0;
    }

    filesystem_t *fs;
    int fid = 0;
    F *f = next_available_file(&fid);

    char *maybe_mount = strdup(path);
    int i = 0;
    while(hashmap_get(mounts, maybe_mount, (void **)&fs)) {
        if (maybe_mount[1] == 0) {
            kprintf("could not find mount in %13s\n", path);
            kfree(maybe_mount);
            return 0;
        }
        char *next_attempt = dirname(maybe_mount);
        kfree(maybe_mount);
        maybe_mount = next_attempt;
    }

    int pathlen = strlen(path);
    int mountlen = strlen(maybe_mount);
    kfree(maybe_mount);

    F_err (*OPEN)(F *, char *, uint16_t) = NULL;
    char *PATH;

    if (pathlen == mountlen) {
        PATH = strdup("");
        OPEN = fs->d_open;
    } else {
        PATH = kmalloc(pathlen - mountlen + 1);
        if (!PATH) {
            return 0;
        }
        memcpy(PATH, &path[mountlen-1], pathlen-mountlen);
        PATH[pathlen-mountlen] = 0;
        switch(fs->node_type(PATH)) {
            case FILE:
            case BLOCK_DEVICE:
                OPEN = fs->f_open;
                break;
            case DIRECTORY:
                OPEN = fs->d_open;
                break;
            case INVALID:
                if (FLAGS && CREATE_ON_OPEN) {
                    OPEN = fs->f_open;
                } else {
                    kfree(PATH);
                    return 0;
                }
                break;
        }
    }

    if (OPEN(f, PATH, FLAGS)) {
        kfree(PATH);
        return 0;
    }

    f->fs = fs;
    f->opened_as = strdup(path);
    kfree(PATH);
    return fid;
}

int read(int fd, void *dest, uint64_t bytes) {
    if (!files[fd].__open__) {
        return 0;
    }

    uint64_t read;
    if (files[fd].fs->f_read(&files[fd], dest, bytes, &read)) {
        return 0;
    }

    return read;
}

int write(int fd, void *src, uint64_t bytes) {
    if (!files[fd].__open__) {
        return 0;
    }
    uint64_t wrote;
    int err = files[fd].fs->f_write(&files[fd], src, bytes, &wrote);
    if (err) {
        return 0;
    }

    return wrote;
}

int close(int fd) {
    F_err (*CLOSE)(F *);
    if (files[fd].__open__) {
        switch(files[fd].__type__) {
            case DIRECTORY:
                CLOSE = files[fd].fs->d_close;
                break;
            case FILE:
                CLOSE = files[fd].fs->f_close;
                break;
            default:
                CLOSE = files[fd].fs->f_close;
                break;
        }

        int x = CLOSE(&files[fd]);
        if (!x) {
            kfree(files[fd].opened_as);
        }
        return x;
    }
    return 1;
}

int mkdir(char *dir) {
    if (!dir) {
        return 0;
    }

    if (dir[0] != '/') {
        return 0;
    }

    char *parent = dirname(dir);
    if (!parent) {
        return 0;
    }

    char *child = basename(dir);
    if (!child) {
        goto kill_parent;
    }

    int fd = open(parent, 0);
    int ret = 0;
    if (!fd) {
        goto failed;
    }

    if (files[fd].fs->d_mkdir(&files[fd], child)) {
        ret = 1;
    }
    close(fd);

failed:
    kfree(child);

kill_parent:
    kfree(parent);
}

int scan_dir(int fd, char **name) {
    if (files[fd].__open__) {
        F_err e = files[fd].fs->d_next(&files[fd], name);
        if (e) {
            return e;
        }
        if (*name != NULL) {
            return 0;
        }
    }
    return 1;
}


int countback_to(char *s, int len, char x) {
    int ct = 0;
    while(len && s[len] != x) {
        len--;
        ct++;
    }
    return ct;
}

char *basename(char *path) {
    int s = strlen(path);
    int baselen = countback_to(path, s-1, '/');
    char *result = kcmalloc(sizeof(char), baselen+1);
    if (!result) {
        return NULL;
    }

    memcpy(result, &path[s-baselen], baselen);
    return result;
}

char *dirname(char *path) {
    int s = strlen(path);
    int baselen = countback_to(path, s-1, '/');
    if (baselen == s-1) {
        return strdup("/");
    }
    char *result = kcmalloc(sizeof(char), s-baselen);
    if (!result) {
        return NULL;
    }

    memcpy(result, path, s-baselen-1);
    result[s-baselen-1] = 0;
    return result;
}

F_type node_type(char *path) {
    int f = open(path, 0);
    if (!f) {
        return false;
    }
    F_type result = files[f].__type__;
    close(f);
    return result;
}

void repeat(int indent, char *c) {
    while(indent --> 0) {
        kprintf(c);
    }
}

void _tree(char *path, int indent) {
    repeat(indent, "  ");
    kprintf("(d) %03s\n", path);

    int f = open(path, 0);
    if (!f) {
        return;
    }

    char *ent = NULL;
    while (!scan_dir(f, &ent)) {
        char *e = smart_join(path, ent, '/');
        switch(node_type(e)) {
            case FILE:
                repeat(indent+1, "  ");
                kprintf("(f) %03s\n", e, ent);
                break;
            case INVALID:
                repeat(indent+1, "  ");
                kprintf("(?) %03s\n", e);
                break;
            case SYMLINK:
                repeat(indent+1, "  ");
                kprintf("(l) %03s\n", e);
                break;
            case BLOCK_DEVICE:
                repeat(indent+1, "  ");
                kprintf("(b) %03s\n", e);
                break;
            case DIRECTORY:
                _tree(e, indent+1);
                break;
        }
        kfree(e);
    }
    close(f);
}

void tree(char *path) {
    _tree(path, 0);
}

int delete(char *path) {
    char *base = dirname(path);
    if (!base) {
        return 1;
    }

    int fd = open(base, 0);
    kfree(base);

    if (!fd) {
        return 1;
    }

    char *filename = basename(path);
    if (!filename) {
        close(fd);
        return 1;
    }

    int result = files[fd].fs->d_delete(&files[fd], filename);
    close(fd);
    return result;
}

void show() {
    for(int i=1; i<MAX_FILES; i++) {
        if (files[i].__open__) {
            kprintf("fd #%03i opened as %03s\n", i, files[i].opened_as);
        }
    }
}


char *F_err_str[] = {
    "NO ERROR",
    "FILE_NOT_FOUND",
    "ARGUMENT_ERROR",
    "NOT_IMPLEMENTED_ERROR",
    "FILESYSTEM_ERROR",
    "FILE_IN_USE",
    "DIRECTORY_NOT_EMPTY"
};

char *err2msg(F_err err) {
    return F_err_str[err];
}