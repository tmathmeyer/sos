#include <devfs.h>
#include <fs.h>
#include <reefs.h>
#include <vfs.h>
#include <mmu.h>
#include <fs.h>
#include <path.h>
#include <alloc.h>
#include <libk.h>

fs_t *fs = NULL;

void devfs_put_device(char *name, struct ata_device *ata) {
    fs_file_t _file;
    fs_file_t *file = &_file;

    char *f_name = path_join("/", name);
    _make_fs_entry(f_name, F_OPT_TYPE_FILE, fs);
    FS_ERROR err = fs->open(fs, f_name, file, F_OPT_CREATE|F_OPT_TYPE_FILE);
    
    file->write(file, "model: ", 7);
    file->write(file, ata->identity.model, min(40, strlen(ata->identity.model)));
    file->write(file, "\n", 1);
    file->write(file, "firmware: ", 10);
    file->write(file, ata->identity.firmware, min(8, strlen(ata->identity.firmware)));
    file->write(file, "\n", 1);
    file->write(file, "serial: ", 8);
    file->write(file, ata->identity.serial, min(20, strlen(ata->identity.serial)));
    
    kfree(f_name);
}

void devfs_init(void) {
	fs = new_mem_reefs(4096 * 30);
    make_fs_entry("/dev", F_OPT_TYPE_MOUNT);
    mount("/dev", fs);
}

