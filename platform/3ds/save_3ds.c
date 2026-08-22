#define _POSIX_C_SOURCE 200809L

#include "platform/save.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SAVE_PARENT "sdmc:/3ds"
#define SAVE_ROOT SAVE_PARENT "/pokeplatinum"
#define SAVE_PATH_LENGTH 256

static bool sSaveReady;

static bool PlatformSave_IsValidPath(const char *path)
{
    if (path == NULL || path[0] == '\0' || strstr(path, "..") != NULL) {
        return false;
    }
    for (const char *character = path; *character != '\0'; character++) {
        if (*character == '/' || *character == '\\') {
            return false;
        }
    }
    return true;
}

static bool PlatformSave_BuildPath(char *destination, size_t capacity, const char *path,
    const char *suffix)
{
    int length;

    if (!sSaveReady || !PlatformSave_IsValidPath(path)) {
        return false;
    }
    length = snprintf(destination, capacity, "%s/%s%s", SAVE_ROOT, path, suffix);
    return length > 0 && (size_t)length < capacity;
}

static bool PlatformSave_FileExists(const char *path)
{
    struct stat status;
    return stat(path, &status) == 0 && S_ISREG(status.st_mode);
}

static bool PlatformSave_Recover(const char *path)
{
    char destination[SAVE_PATH_LENGTH];
    char backup[SAVE_PATH_LENGTH];

    if (!PlatformSave_BuildPath(destination, sizeof(destination), path, "")
        || !PlatformSave_BuildPath(backup, sizeof(backup), path, ".bak")) {
        return false;
    }
    if (PlatformSave_FileExists(destination)) {
        remove(backup);
        return true;
    }
    if (PlatformSave_FileExists(backup)) {
        return rename(backup, destination) == 0;
    }
    return true;
}

bool PlatformSave_Init(void)
{
    if (mkdir(SAVE_PARENT, 0777) != 0 && errno != EEXIST) {
        return false;
    }
    if (mkdir(SAVE_ROOT, 0777) != 0 && errno != EEXIST) {
        return false;
    }
    sSaveReady = true;
    return true;
}

void PlatformSave_Shutdown(void)
{
    sSaveReady = false;
}

bool PlatformSave_Read(const char *path, void *destination, size_t size)
{
    char sourcePath[SAVE_PATH_LENGTH];
    FILE *file;
    bool success;

    if (destination == NULL || size == 0 || !PlatformSave_Recover(path)
        || !PlatformSave_BuildPath(sourcePath, sizeof(sourcePath), path, "")) {
        return false;
    }
    file = fopen(sourcePath, "rb");
    if (file == NULL) {
        return false;
    }
    success = fread(destination, 1, size, file) == size && fgetc(file) == EOF;
    success = fclose(file) == 0 && success;
    return success;
}

bool PlatformSave_Stage(const char *path, const void *source, size_t size)
{
    char temporaryPath[SAVE_PATH_LENGTH];
    FILE *file;
    bool success;

    if (source == NULL || size == 0
        || !PlatformSave_BuildPath(temporaryPath, sizeof(temporaryPath), path, ".tmp")) {
        return false;
    }
    file = fopen(temporaryPath, "wb");
    if (file == NULL) {
        return false;
    }
    success = fwrite(source, 1, size, file) == size;
    success = fflush(file) == 0 && success;
    if (success) {
        success = fsync(fileno(file)) == 0;
    }
    success = fclose(file) == 0 && success;
    if (!success) {
        remove(temporaryPath);
    }
    return success;
}

bool PlatformSave_Commit(const char *path)
{
    char destination[SAVE_PATH_LENGTH];
    char temporary[SAVE_PATH_LENGTH];
    char backup[SAVE_PATH_LENGTH];
    bool hadDestination;

    if (!PlatformSave_Recover(path)
        || !PlatformSave_BuildPath(destination, sizeof(destination), path, "")
        || !PlatformSave_BuildPath(temporary, sizeof(temporary), path, ".tmp")
        || !PlatformSave_BuildPath(backup, sizeof(backup), path, ".bak")
        || !PlatformSave_FileExists(temporary)) {
        return false;
    }

    hadDestination = PlatformSave_FileExists(destination);
    remove(backup);
    if (hadDestination && rename(destination, backup) != 0) {
        return false;
    }
    if (rename(temporary, destination) != 0) {
        if (hadDestination) {
            rename(backup, destination);
        }
        return false;
    }
    remove(backup);
    return true;
}

bool PlatformSave_WriteAtomic(const char *path, const void *source, size_t size)
{
    return PlatformSave_Stage(path, source, size) && PlatformSave_Commit(path);
}

bool PlatformSave_HasStagedWrite(const char *path)
{
    char temporary[SAVE_PATH_LENGTH];

    return PlatformSave_BuildPath(temporary, sizeof(temporary), path, ".tmp")
        && PlatformSave_FileExists(temporary);
}
