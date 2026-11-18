#include "fs.h"
#include "string.h"

static fs_file files[FS_VOLUMES][FS_MAX_FILES];
static int counts[FS_VOLUMES];
static const char *volume_names[FS_VOLUMES] = {
    "SYSTEM",
    "DISC"
};

static void copy_name(char *dst, const char *src)
{
    int i = 0;

    while (src[i] && i < FS_MAX_NAME - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static fs_file *create_file(int volume, const char *name)
{
    fs_file *file;

    if (volume < 0 || volume >= FS_VOLUMES || counts[volume] >= FS_MAX_FILES)
        return 0;

    file = &files[volume][counts[volume]++];
    memset(file, 0, sizeof(*file));
    copy_name(file->name, name);
    return file;
}

void fs_init(void)
{
    fs_file *file;

    counts[0] = 0;
    counts[1] = 0;

    file = create_file(0, "readme.txt");
    if (file)
        file->size = (int)strlen("matenOS file system\n"
                                 "SYSTEM volume lives in RAM.\n"
                                 "Open a file and press Save in Notepad.\n"
                                 "Use Files to copy it to DISK.\n");
    if (file)
        memcpy(file->data,
               "matenOS file system\n"
               "SYSTEM volume lives in RAM.\n"
               "Open a file and press Save in Notepad.\n"
               "Use Files to copy it to DISK.\n",
               file->size);

    file = create_file(0, "notes.txt");
    if (file) {
        const char *text = "todo:\n- try the calculator\n- draw something\n- edit this file\n";
        file->size = (int)strlen(text);
        memcpy(file->data, text, file->size);
    }

    file = create_file(1, "manual.txt");
    if (file) {
        const char *text = "This DISK volume is a virtual image.\n"
                           "Files are read from matenos-disk.img.\n";
        file->size = (int)strlen(text);
        memcpy(file->data, text, file->size);
    }


}

const char *fs_volume_name(int volume)
{
    if (volume < 0 || volume >= FS_VOLUMES)
        return "?";
    return volume_names[volume];
}

int fs_count(int volume)
{
    if (volume < 0 || volume >= FS_VOLUMES)
        return 0;
    return counts[volume];
}

fs_file *fs_at(int volume, int index)
{
    if (volume < 0 || volume >= FS_VOLUMES || index < 0 || index >= counts[volume])
        return 0;
    return &files[volume][index];
}

fs_file *fs_find(int volume, const char *name)
{
    int i;

    for (i = 0; i < fs_count(volume); i++) {
        if (!strcmp(files[volume][i].name, name))
            return &files[volume][i];
    }
    return 0;
}

fs_file *fs_create(int volume, const char *name)
{
    fs_file *existing = fs_find(volume, name);
    if (existing)
        return existing;
    return create_file(volume, name);
}

int fs_delete(int volume, const char *name)
{
    int i;

    if (volume < 0 || volume >= FS_VOLUMES)
        return 0;

    for (i = 0; i < counts[volume]; i++) {
        if (!strcmp(files[volume][i].name, name)) {
            int j;
            for (j = i; j < counts[volume] - 1; j++)
                files[volume][j] = files[volume][j + 1];
            counts[volume]--;
            return 1;
        }
    }
    return 0;
}

int fs_write(int volume, const char *name, const void *data, int size)
{
    fs_file *file = fs_find(volume, name);

    if (!file)
        file = fs_create(volume, name);
    if (!file)
        return 0;
    if (size > FS_MAX_DATA)
        size = FS_MAX_DATA;
    file->size = size;
    memcpy(file->data, data, (size_t)size);
    return 1;
}

int fs_copy(int from_volume, const char *name, int to_volume)
{
    fs_file *source = fs_find(from_volume, name);
    fs_file *target;

    if (!source || to_volume < 0 || to_volume >= FS_VOLUMES)
        return 0;
    target = fs_create(to_volume, name);
    if (!target)
        return 0;
    target->size = source->size;
    memcpy(target->data, source->data, (size_t)source->size);
    return 1;
}


