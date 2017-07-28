#include <devfs.h>
#include <fs.h>
#include <reefs.h>
#include <vfs.h>
#include <mmu.h>
#include <fs.h>
#include <path.h>
#include <alloc.h>
#include <libk.h>
#include <sorts.h>

fs_t *fs = NULL;
bsl_t *diskIDs = NULL;

void devfs_put_device(char *name, struct ata_device *ata) {
    fs_file_t _file;
    fs_file_t *file = &_file;

    char *dir_name = path_join("/", name);
    _make_fs_entry(dir_name, F_OPT_TYPE_DIR, fs);
    char *info_name = path_join(dir_name, "info");
    char *IDnum_name = path_join(dir_name, "__id");
    
    FS_ERROR err = fs->open(fs, info_name, file, F_OPT_CREATE|F_OPT_TYPE_FILE);
    
    file->write(file, "model: ", 7);
    file->write(file, ata->identity.model, min(40, strlen(ata->identity.model)));
    file->write(file, "\n", 1);
    file->write(file, "firmware: ", 10);
    file->write(file, ata->identity.firmware, min(8, strlen(ata->identity.firmware)));
    file->write(file, "\n", 1);
    file->write(file, "serial: ", 8);
    file->write(file, ata->identity.serial, min(20, strlen(ata->identity.serial)));
    file->write(file, "\n", 1);
    char msg[512] = {0};
    sprintf(msg, "sectors: %i\nvalid ext data: %i\nrw mult size: %i\nsectors 28: %i\nsectors 48: %i",
            ata->identity.sectors_per_int,
            ata->identity.size_of_rw_mult,
            ata->identity.sectors_28,
            ata->identity.sectors_48);
    file->write(file, msg, strlen(msg));

    uint64_t ID = bsl_add(diskIDs, ata);
    err = fs->open(fs, IDnum_name, file, F_OPT_CREATE|F_OPT_TYPE_FILE);
    file->write(file, &ID, sizeof(ID));
    

    kfree(dir_name);
    kfree(info_name);
    kfree(IDnum_name);
}

struct ata_device *get_ata_by_dev_dir(char *dev_dir) {
    char *id_name = path_join(dev_dir, "__id");
    fs_file_t _file;
    fs_file_t *file = &_file;

    uint64_t id = 0;
    FS_ERROR err = open_stat(id_name, NULL, file);
    if (err) {
        return NULL;
    }
    file->read(file, &id, sizeof(id));
    struct ata_device *result = bsl_get(diskIDs, id);
    kfree(id_name);
    return result;
}

void devfs_init(void) {
	fs = new_mem_reefs(4096 * 30);
    diskIDs = new_bsl();
    make_fs_entry("/dev", F_OPT_TYPE_MOUNT);
    mount("/dev", fs);
}

