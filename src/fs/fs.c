#include <filesystem.h>
#include <mmu.h>
#include <libk.h>
#include <kio.h>
#include <devices.h>

typedef struct {
	char name[120];
	inode_t *inode;
}__attribute__((packed)) mp_t;

static mp_t *mounts = NULL;

mp_t *get_mounts() {
	if (!mounts) {
		mounts = (void *)starting_address(allocate_full_page());
		memset(mounts, 0, PAGE_SIZE);
	}
	return mounts;
}

/* returns an inode if there is a corresponding mount or NULL */
inode_t *maybe_mount_point(mp_t *mp_page, char *path) {
	for(int i=0; i<PAGE_SIZE/sizeof(mp_t); i++) {
		if (mp_page[i].inode && !strncmp(path, mp_page[i].name, 120)) {
			return mp_page[i].inode;
		}
	}
	return NULL;
}

/* returns the highest level inode for a file */
inode_t *get_mount_for_file(char *path, char **rest) {
	mp_t *mp_page = get_mounts();
	uint64_t len_w_nt = strlen(path)+1;
	uint64_t last_null_set = 0;
	char last_chr = 0;

	while(len_w_nt) {
		inode_t *res = maybe_mount_point(mp_page, path);
		if (res) {
			*rest = &path[len_w_nt-1];
			return res;
		}
		while(path[len_w_nt] != '/') {
			len_w_nt--;
		}
		if (last_null_set) {
			path[last_null_set] = last_chr;
		}
		last_null_set = len_w_nt;
		last_chr = path[len_w_nt];
		path[len_w_nt] = 0;
	}
	path[0] = last_chr;
	*rest = &path[0];
	return maybe_mount_point(mp_page, "/");
}

node_type get_type(char *location) {
	return TYPE_DIR;
}

/* mount a filesystem (TODO: return shit) */
void mount_fs(inode_t *entry, char *location) {
	if (get_type(location) == TYPE_DIR && strlen(location) < 120) {
		mp_t *mounts = get_mounts();
		for(int i=0; i<PAGE_SIZE/sizeof(mp_t); i++) {
			if (!mounts[i].inode) {
				mounts[i].inode = entry;
				memcpy(mounts[i].name, location, strlen(location)+1);
			}
		}
	}
}




bool inode_get_entry(inode_t *node, uint64_t *te, uint32_t *slot, char *path) {
	for(uint64_t ent=0; ent < node->data.entry_count; ent++) {
		if (!strncmp(node->data.entries[ent].name, path, inode_name_max)) {
			*te = node->data.entries[ent].table_entry;
			*slot = node->data.entries[ent].table_slot;
			return true;
		}
	}
	return false;
}

FS_ERROR inode_traverse(inode_t *node, char *path, uint64_t *te, uint32_t *slot) {
	int next_slash = 0;
	while(path[next_slash] && path[next_slash] != '/') {
		next_slash++;
	}
	char backup = path[next_slash];
	path[next_slash] = 0;

	uint64_t _entry;
	uint32_t _slot;
	if (!inode_get_entry(node, &_entry, &_slot, path)) {
		path[next_slash] = backup;
		return FILE_NOT_FOUND;
	}
	path[next_slash] = backup;

	node_type type;
	FS_ERROR err = node->fs->get_type(node->fs, _entry, _slot, &type);
	if (err) {
		return err;
	}

	if (type == TYPE_FILE) {
		*te = _entry;
		*slot = _slot;
		return NO_ERROR;
	}

	if (type != TYPE_DIR) {
		return ERROR_CRIT;
	}

	uint64_t size;
	err = node->fs->get_size(node->fs, _entry, _slot, &size);
	if (err) {
		return err;
	}
	if (size > PAGE_SIZE) {
		return ERROR_CRIT;
	}

	inode_t *addr = (inode_t *)(starting_address(allocate_full_page()));
	memset(addr, 0, PAGE_SIZE);
	err = node->fs->read_dir_at(node->fs, _entry, _slot, addr);
	if (err) {
		release_full_page(containing_address((uint64_t)addr));
		return err;
	}
	addr->fs = node->fs;
	err = inode_traverse(addr, path+next_slash+1, te, slot);
	release_full_page(containing_address((uint64_t)addr));
	return err;
}




FS_ERROR file_read(char *name, void *data, uint64_t len, uint64_t *read) {
	char *innerfile;
	inode_t *node = get_mount_for_file(name, &innerfile);
	if (!node) {
		return FILE_NOT_FOUND;
	}
	uint64_t te;
	uint32_t slot;
	FS_ERROR err = inode_traverse(node, innerfile, &te, &slot);
	if (err) {
		return err;
	}

	kprintf("inner path = \"%04s\"\n", innerfile);
	return node->fs->read_file_at(node->fs, te, slot, len, read, data);
}