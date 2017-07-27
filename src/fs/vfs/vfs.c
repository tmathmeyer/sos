#include <vfs.h>
#include <fs.h>
#include <reefs.h>
#include <alloc.h>
#include <libk.h>
#include <kio.h>
#include <path.h>
#include <sorts.h>

char *FS_ERROR_STRING[5] = {
    "No Error",
    "File Not Found",
    "Disk Read Error",
    "Disk Write Error",
    "Not a Directory"
};

char *FTYPE_STR[4] = {
    "",
    "FIL",
    "DIR",
    "MNT"
};

fs_t *root = NULL;
bsl_t *sysmap = NULL;

void vfs_init(void) {
	root = new_mem_reefs(4096 * 3);
    sysmap = new_bsl();
}

fs_t *get_mount_by_id(int id) {
    return bsl_get(sysmap, id);
}

void make_fs_entry(char *path, int type) {
    _make_fs_entry(path, type, root);
}

void _make_fs_entry(char *path, int type, fs_t *fs) {
    char *path_so_far = kmalloc(2);
    memcpy(path_so_far, "/", 2);
    
    char *part;
    int remaining;
    path_iterate(path, part, remaining) {
        char *n_path = path_join(path_so_far, part);
        kfree(path_so_far);
        path_so_far = n_path;

        fs_metadata_t meta;
        FS_ERROR err = fs->stat(fs, path_so_far, &meta);
        if (err) {
            if (remaining == 0) {
                fs_file_t entry;
                fs->open(fs, path_so_far, &entry, F_OPT_CREATE|type);
                if (type == F_OPT_TYPE_MOUNT) {
                    uint64_t data = 0;
                    entry.write(&entry, &data, sizeof(uint64_t));
                }
            } else {
                kprintf("Error making mount, Stat failed\n");
            }
            kfree(path_so_far);
            goto_safe(ret_err);
        }
        
        if (meta.type == FTYPE_FILE) {
            kprintf("Error, file already exists\n");
            kfree(path_so_far);
            goto_safe(ret_err);
        }
        if (meta.type == FTYPE_DIR) {
            continue;
        }
        if (meta.type == FTYPE_MOUNT) {
            fs_file_t file;
            err = fs->open(fs, path, &file, 0);
            
            int fs_id;
            file.read(&file, &fs_id, sizeof(int));
            
            fs = get_mount_by_id(fs_id);
            kfree(path_so_far);
            path_so_far = kmalloc(2);
            memcpy(path_so_far, "/", 2);
        }
    }
    kfree(path_so_far);
ret_err:
    (void) "Loop jump-out stub";
}

FS_ERROR open_stat(char *path, fs_metadata_t *meta, fs_file_t *file) {
    if (!meta) {
        fs_metadata_t __meta;
        meta = &__meta;
    }

    if (!file) {
        fs_file_t __file;
        file = &__file;
    }

    char *path_so_far = kmalloc(2);
    memcpy(path_so_far, "/", 2);
    
    fs_t *fs = root;
    char *part;
    int remaining;
    FS_ERROR err;
    path_iterate(path, part, remaining) {
        char *n_path = path_join(path_so_far, part);
        kfree(path_so_far);
        path_so_far = n_path;

        err = fs->stat(fs, path_so_far, meta);
        if (err) {
            kfree(path_so_far);
            goto_safe(ret_err);
        }
        if (remaining == 0) {
            err = fs->open(fs, path_so_far, file, 0);
            if (meta->type == FTYPE_MOUNT) {
                 uint64_t fs_id;
                 file->read(file, &fs_id, sizeof(fs_id));
                 if (fs_id) {
                     fs = get_mount_by_id(fs_id);
                     err = fs->stat(fs, "/", meta);
                     err = fs->open(fs, "/", file, 0);
                 }
            }
            kfree(path_so_far);
            goto_safe(ret_err);
        }
        
        if (meta->type == FTYPE_FILE) {
            kfree(path_so_far);
            err = NOT_A_DIRECTORY;
            goto_safe(ret_err);
        }
        if (meta->type == FTYPE_DIR) {
            continue;
        }
        if (meta->type == FTYPE_MOUNT) {
            fs_file_t _file;
            err = fs->open(fs, path_so_far, &_file, 0);
            
            uint64_t fs_id;
            _file.read(&_file, &fs_id, sizeof(uint64_t));
            
            fs = get_mount_by_id(fs_id);
            kfree(path_so_far);
            path_so_far = kmalloc(2);
            memcpy(path_so_far, "/", 2);
        }
    }
    err = fs->stat(fs, path_so_far, meta);
    err = fs->open(fs, path_so_far, file, 0);
    kfree(path_so_far);
ret_err:
    return err;
}

void mount(void *path, fs_t *fs) {
    fs_metadata_t meta;
    fs_file_t file;
    FS_ERROR err = open_stat(path, &meta, &file);
    if (err) {
        kprintf("open_stat(%05s) error code(%03i) = %03s (%05i)\n",
                path, err, FS_ERROR_STRING[err], __LINE__);
        return;
    }
    if (meta.type != FTYPE_MOUNT) {
        kprintf("Not a mount point\n");
        return;
    }
    uint64_t fs_id;
    file.read(&file, &fs_id, sizeof(uint64_t));
    if (fs_id != 0) {
        kprintf("already an occupied mount point!\n");
        return;
    }
    fs_id = bsl_add(sysmap, fs);
    file.rewind(&file);
    file.write(&file, &fs_id, sizeof(uint64_t));
}


void ls(char *__path) {
    if (__path[0] == '/') {
        char *___path = kmalloc(strlen(__path)+1);
        memcpy(___path, __path, strlen(__path)+1);
        __path = ___path;
    } else {
        __path = path_join("/", __path);
    }


    fs_file_t _file;
    fs_file_t *file = &_file;
    fs_metadata_t meta;
    FS_ERROR err = open_stat(__path, &meta, file);
	if (err) {
        kprintf("open_stat(%05s) error code = %03s (%05i)\n", __path, FS_ERROR_STRING[err], __LINE__);
        goto fail;
	}
	if (meta.type != FTYPE_DIR) {
		kprintf("type(%05s) is not a directory(%03i)\n", __path, meta.type);
        goto fail;
	}

	directory_t dir;
	file->read(file, &dir, sizeof(dir));
	for(int i=0; i<dir.entry_count; i++) {
		dirent_t ent;
		file->read(file, &ent, sizeof(ent));
		char name[ent.name_length+1];
		file->read(file, name, ent.name_length);
        char *subpath = path_join(__path, name);
        err = open_stat(subpath, &meta, NULL);
		if (err) {
            kprintf("open_stat(%05s) error code = %03s (%05i)\n", subpath, FS_ERROR_STRING[err], __LINE__);
            kfree(subpath);
            goto fail;
		}
        kfree(subpath);
		kprintf("%03s %03c%03c%03c %03i %03s\n",
			FTYPE_STR[meta.type],
			meta.rwx&0x4?'r':'-',
			meta.rwx&0x2?'w':'-',
			meta.rwx&0x1?'x':'-',
			meta.size_bytes,
			name);
	}

fail:
    kfree(__path);
    return;
}

void cat(char *path) {

    fs_metadata_t meta;
    fs_file_t file;
    FS_ERROR err = open_stat(path, &meta, &file);
    if (err) {
        kprintf("open_stat(%05s) error code = %03s (%05i)\n", path, FS_ERROR_STRING[err], __LINE__);
        return;
    }
    if (meta.type != FTYPE_FILE) {
        kprintf("not a file");
        return;
    }
    char data[513];
    int len = 0;
    while(len = file.read(&file, data, 512)) {
        data[len] = 0;
        kprintf("%07s", data); 
    }
    kprintf("\n");

}
