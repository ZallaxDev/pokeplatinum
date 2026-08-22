#ifndef POKEPLATINUM_3DS_NITRO_FS_H
#define POKEPLATINUM_3DS_NITRO_FS_H

#include "pokeplatinum_compat.h"

typedef u32 FSOverlayID;

typedef enum FSSeekFileMode {
    FS_SEEK_SET,
    FS_SEEK_CUR,
    FS_SEEK_END,
} FSSeekFileMode;

typedef struct FSFile {
    void *handle;
} FSFile;

void FS_InitFile(FSFile *file);
BOOL FS_OpenFile(FSFile *file, const char *path);
BOOL FS_CloseFile(FSFile *file);
s32 FS_ReadFile(FSFile *file, void *dest, s32 length);
BOOL FS_SeekFile(FSFile *file, s32 offset, FSSeekFileMode origin);

#endif // POKEPLATINUM_3DS_NITRO_FS_H
