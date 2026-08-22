#include "platform/filesystem.h"

#include <stdio.h>

static bool BuildRomfsPath(const char *path, char *fullPath, size_t capacity)
{
    int length;

    if (path == NULL || path[0] == '/' || capacity == 0) {
        return false;
    }

    length = snprintf(fullPath, capacity, "romfs:/%s", path);
    return length > 0 && (size_t)length < capacity;
}

bool PlatformFile_GetSize(const char *path, size_t *size)
{
    char fullPath[256];
    FILE *file;
    long length;

    if (size == NULL || !BuildRomfsPath(path, fullPath, sizeof(fullPath))) {
        return false;
    }

    file = fopen(fullPath, "rb");
    if (file == NULL) {
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }

    length = ftell(file);
    fclose(file);
    if (length < 0) {
        return false;
    }

    *size = (size_t)length;
    return true;
}

bool PlatformFile_Read(const char *path, void *destination, size_t size)
{
    char fullPath[256];
    FILE *file;
    bool success;

    if ((destination == NULL && size != 0) || !BuildRomfsPath(path, fullPath, sizeof(fullPath))) {
        return false;
    }

    file = fopen(fullPath, "rb");
    if (file == NULL) {
        return false;
    }

    success = fread(destination, 1, size, file) == size && fgetc(file) == EOF;
    fclose(file);
    return success;
}
