#ifndef POKEPLATINUM_PLATFORM_FILESYSTEM_H
#define POKEPLATINUM_PLATFORM_FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>

bool PlatformFile_GetSize(const char *path, size_t *size);
bool PlatformFile_Read(const char *path, void *destination, size_t size);

#endif
