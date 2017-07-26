#include <vfs.h>
#include <fs.h>
#include <reefs.h>
#include <alloc.h>
#include <libk.h>
#include <kio.h>
#include <path.h>

















// Mount point handler

struct mount_point_tree {
	char path_name[47]; uint8_t child_count;
	fs_t *fs;
	struct mount_point_tree **children;
};

struct mount_point_tree *root = NULL;

void vfs_init(void) {
	fs_t *fs = new_mem_reefs(4096 * 3);

	root = kmalloc(sizeof(struct mount_point_tree));
	root->fs = fs;
	root->children = NULL;
	root->child_count = 0;
	memcpy(root->path_name, "", 1);
}

fs_t *get_mount_by_path(struct mount_point_tree *mpt, char *path, char **ROP) {
	if (ROP == NULL) {
		return NULL;
	}
	*ROP = path;
    path += 1;
	if (mpt->child_count == 0) {
		return mpt->fs;
	}
	if (mpt->children == NULL) {
		return mpt->fs;
	}
	for(uint64_t i=0; i<mpt->child_count; i++) {
        char *_path = mpt->children[i]->path_name;
		uint64_t path_len = strlen(_path);
		if (!strncmp(path, _path, path_len)) {
			return get_mount_by_path(mpt, path+path_len, ROP);
		}
	}
    return mpt->fs;
}

struct mount_point_tree *create_node_in_tree(struct mount_point_tree *mpt, char *part) {
	uint64_t old_count = mpt->child_count;
	struct mount_point_tree **new_children = kmalloc(sizeof(void *) * (old_count+1));
	if (mpt->children) {
		memcpy(new_children, mpt->children, old_count*sizeof(void *));
        kfree(mpt->children);
	}
    mpt->children = new_children;
	new_children[old_count] = kmalloc(sizeof(struct mount_point_tree));
	new_children[old_count]->children = NULL;
	new_children[old_count]->child_count = 0;
	new_children[old_count]->fs = NULL;
	memset(new_children[old_count]->path_name, 0, 47);
	memcpy(new_children[old_count]->path_name, part, strlen(part));
	mpt->child_count = old_count+1;
}

void mount_fs_by_path(fs_t *fs, char *path) {
	struct mount_point_tree *mpt = root;
	char *part;
	uint64_t remaining = 0;
	path_iterate(path, part, remaining) {
        kprintf("mounting path=[%13s], part=[%25s]\n", path, part);
		for(uint64_t i=0; i<mpt->child_count; i++) {
			char *_path = mpt->children[i]->path_name;
			uint64_t path_len = strlen(_path);
			if (!strncmp(path, _path, path_len)) {
				mpt = mpt->children[i];
				goto EOL;
			}
		}
FS_ERROR reefs_open(fs_t *fs, char *path, fs_file_t *entry, FLAGS flags) {
        fs_file_t
        fs->open(part, F_OPT_DIR|F_OPT_CREATE
		mpt = create_node_in_tree(mpt, part);
EOL:
		(void) "End of Loop. go back and try again!";
	}
	mpt->fs = fs;
}





fs_metadata_t stat(fs_t *fs, char *path, FS_ERROR *err) {
	fs_metadata_t meta;
	*err = fs->stat(fs, path, &meta);
	return meta;
}

void ls(char *__path) {
    if (__path[0] == '/') {
        char *___path = kmalloc(strlen(__path)+1);
        memcpy(___path, __path, strlen(__path)+1);
        __path = ___path;
    } else {
        __path = path_join("/", __path);
    }
	char *path = NULL;
	fs_t *fs = get_mount_by_path(root, __path, &path);

	FS_ERROR err;
	fs_metadata_t meta = stat(fs, path, &err);
	if (err) {
		kprintf("STAT error code = %03s\n", FS_ERROR_STRING[err]);
        goto fail;
	}
	if (meta.type != FTYPE_DIR) {
		kprintf("not a directory");
        goto fail;
	}

	fs_file_t _file;
	fs_file_t *file = &_file;
	err = fs->open(fs, path, file, 0);
	if (err) {
		kprintf("OPEN error code = %03s\n", FS_ERROR_STRING[err]);
        goto fail;
	}

	directory_t dir;
	file->read(file, &dir, sizeof(dir));
	char *copyto = path + strlen(path);
	for(int i=0; i<dir.entry_count; i++) {
		dirent_t ent;
		
		file->read(file, &ent, sizeof(ent));
		char name[ent.name_length];
		file->read(file, name, ent.name_length);
		if (copyto[-1] == '/') {
			memcpy(copyto, name, ent.name_length);
		} else {
			copyto[0] = '/';
			memcpy(copyto+1, name, ent.name_length);
		}
		meta = stat(fs, path, &err);
		if (err) {
            kprintf("STAT error code = %03s\n", FS_ERROR_STRING[err]);
            goto fail;
		}
		kprintf("%03s %03c%03c%03c %03i %03s\n",
			FTYPE_STR(meta.type),
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
