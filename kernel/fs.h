#ifndef MATENOS_FS_H
#define MATENOS_FS_H

#include <stdint.h>

#define FS_VOLUMES   2
#define FS_MAX_FILES 12
#define FS_MAX_NAME  20
#define FS_MAX_DATA  2048

typedef struct {
    char name[FS_MAX_NAME];
    int size;
    uint8_t data[FS_MAX_DATA];
} fs_file;

void fs_init(void);
const char *fs_volume_name(int volume);
int fs_count(int volume);
fs_file *fs_at(int volume, int index);
fs_file *fs_find(int volume, const char *name);
fs_file *fs_create(int volume, const char *name);
int fs_delete(int volume, const char *name);
int fs_write(int volume, const char *name, const void *data, int size);
int fs_copy(int from_volume, const char *name, int to_volume);

#endif
