#include "runtime_adapters.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "game_overlay.h"
#include "heap.h"

#define STATIC_OVERLAY_COUNT 128

static BOOL sLoadedOverlays[STATIC_OVERLAY_COUNT];
static u32 sOverlayLoadCount;
static u32 sOverlayUnloadCount;
#ifdef __3DS__
static const char *sFileRoot = "romfs:/";
#else
static const char *sFileRoot = "build-3ds-real/romfs/";
#endif

void RuntimeAdapters_Reset(void)
{
    memset(sLoadedOverlays, 0, sizeof(sLoadedOverlays));
    sOverlayLoadCount = 0;
    sOverlayUnloadCount = 0;
}

void RuntimeAdapters_SetFileRoot(const char *root)
{
    sFileRoot = root;
}

BOOL RuntimeAdapters_IsOverlayLoaded(FSOverlayID overlayID)
{
    return overlayID < STATIC_OVERLAY_COUNT && sLoadedOverlays[overlayID];
}

u32 RuntimeAdapters_GetOverlayLoadCount(void)
{
    return sOverlayLoadCount;
}

u32 RuntimeAdapters_GetOverlayUnloadCount(void)
{
    return sOverlayUnloadCount;
}

void *Heap_Alloc(u32 heapID, u32 size)
{
    (void)heapID;
    return malloc(size);
}

void *Heap_AllocAtEnd(u32 heapID, u32 size)
{
    return Heap_Alloc(heapID, size);
}

void Heap_Free(void *ptr)
{
    free(ptr);
}

BOOL Overlay_LoadByID(const FSOverlayID overlayID, enum OverlayLoadType loadType)
{
    (void)loadType;

    if (overlayID >= STATIC_OVERLAY_COUNT || sLoadedOverlays[overlayID]) {
        return FALSE;
    }

    sLoadedOverlays[overlayID] = TRUE;
    sOverlayLoadCount++;
    return TRUE;
}

void Overlay_UnloadByID(const FSOverlayID overlayID)
{
    if (overlayID < STATIC_OVERLAY_COUNT && sLoadedOverlays[overlayID]) {
        sLoadedOverlays[overlayID] = FALSE;
        sOverlayUnloadCount++;
    }
}

int Overlay_GetLoadDestination(const FSOverlayID overlayID)
{
    (void)overlayID;
    return 0;
}

void FS_InitFile(FSFile *file)
{
    file->handle = NULL;
}

BOOL FS_OpenFile(FSFile *file, const char *path)
{
    char nativePath[512];

    if (snprintf(nativePath, sizeof(nativePath), "%s%s", sFileRoot, path) >= (int)sizeof(nativePath)) {
        return FALSE;
    }

    file->handle = fopen(nativePath, "rb");
    return file->handle != NULL;
}

BOOL FS_CloseFile(FSFile *file)
{
    if (file->handle == NULL) {
        return FALSE;
    }

    BOOL closed = fclose(file->handle) == 0;
    file->handle = NULL;
    return closed;
}

s32 FS_ReadFile(FSFile *file, void *dest, s32 length)
{
    if (file->handle == NULL || length < 0) {
        return -1;
    }

    return (s32)fread(dest, 1, (size_t)length, file->handle);
}

BOOL FS_SeekFile(FSFile *file, s32 offset, FSSeekFileMode origin)
{
    static const int sOrigins[] = { SEEK_SET, SEEK_CUR, SEEK_END };

    if (file->handle == NULL || (u32)origin >= sizeof(sOrigins) / sizeof(sOrigins[0])) {
        return FALSE;
    }

    return fseek(file->handle, offset, sOrigins[origin]) == 0;
}
