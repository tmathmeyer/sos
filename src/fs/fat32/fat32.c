
#include <fs/fs.h>
#include <fs/fat32/fat32.h>
#include <fs/fat32/fat32_fs.h>

filesystem_t *fat32_new_fs(block_device_t device) {


	return NULL;
}

F_type fat32_node_type(filesystem_t *fs, char *name);

F_err fat32_f_open(F *file, char *name, uint16_t mode);
F_err fat32_f_close(F *file);
F_err fat32_f_read(F *file, void *dest, uint64_t to_read, uint64_t *actually_read);
F_err fat32_f_write(F *file, void *src, uint64_t to_write, uint64_t *actually_wrote);
F_err fat32_f_lseek(F *file, uint64_t seek_to);
F_err fat32_f_tell(F *file, uint64_t *buffer_position);
F_err fat32_f_size(F *file, uint64_t *file_size);
bool  fat32_f_eof(F *file);

F_err fat32_d_open(F *dir, char *name, uint16_t mode);
F_err fat32_d_close(F *dir);
F_err fat32_d_next(F *dir, char **name);
F_err fat32_d_rewind(F *dir);
F_err fat32_d_mkdir(F *dir, char *name);
F_err fat32_d_delete(F *dir, char *name);