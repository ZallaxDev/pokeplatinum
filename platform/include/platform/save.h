#ifndef POKEPLATINUM_PLATFORM_SAVE_H
#define POKEPLATINUM_PLATFORM_SAVE_H

#include <stdbool.h>
#include <stddef.h>

bool PlatformSave_Init(void);
void PlatformSave_Shutdown(void);
bool PlatformSave_Read(const char *path, void *destination, size_t size);
bool PlatformSave_Stage(const char *path, const void *source, size_t size);
bool PlatformSave_Commit(const char *path);
bool PlatformSave_WriteAtomic(const char *path, const void *source, size_t size);
bool PlatformSave_HasStagedWrite(const char *path);

#endif
