typedef int fs_t;

typedef
struct mount_point_s {
	struct mount_point_s *parent;
	struct mount_point_s *next;
	char *fs_path;
	fs_t *fs;
} mount_point_t;

mount_point_t *root = NULL;
mount_point_t *tail = NULL;

FS_ERROR_T mount_fs(path_t *path, fs_t *fs) {
	path_t *post_mount;
	mount_point_t *parent_mount = get_post_mount(path, root, &post_mount)
	fs_stat_t info = stat(parent_mount->fs, post_mount);
	if (info.type != FS_TYPE_DIR) {
		return FS_ERROR_NOT_A_DIR;
	}

	tail->next = malloc(sizeof(mount_point_t));
	tail = tail->next;
	tail->next = NULL;
	tail->parent = parent_mount;
	tail->fs_path = path_str(post_mount);
	tail->fs = fs;

	return NO_ERROR;
}

FS_ERROR_T umount_fs(path_t *path) {
	
}