#include "runtime_adapters.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "game_overlay.h"
#include "heap.h"
#include "nitro/rtc.h"

#define STATIC_OVERLAY_COUNT 128

static BOOL sLoadedOverlays[STATIC_OVERLAY_COUNT];
static u32 sOverlayLoadCount;
static u32 sOverlayUnloadCount;
static u32 sRTCReadCount;
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
    sRTCReadCount = 0;
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

u32 RuntimeAdapters_GetRTCReadCount(void)
{
    return sRTCReadCount;
}

static void *ArenaAlloc(u32 size, u32 alignment)
{
    uintptr_t raw = (uintptr_t)malloc(size + alignment - 1);

    if (raw == 0) {
        return NULL;
    }
    return (void *)((raw + alignment - 1) & ~(uintptr_t)(alignment - 1));
}

void *OS_AllocFromMainArenaLo(u32 size, u32 alignment)
{
    return ArenaAlloc(size, alignment);
}

void *OS_AllocFromMainExArenaHi(u32 size, u32 alignment)
{
    return ArenaAlloc(size, alignment);
}

void *OS_AllocFromHeap(OSArenaId arena, OSHeapHandle heap, u32 size)
{
    (void)arena;
    (void)heap;
    return malloc(size);
}

void OS_FreeToHeap(OSArenaId arena, OSHeapHandle heap, void *ptr)
{
    (void)arena;
    (void)heap;
    free(ptr);
}

OSProcMode OS_GetProcMode(void)
{
    return OS_PROCMODE_USER;
}

OSIntrMode OS_DisableInterrupts(void)
{
    return 0;
}

void OS_RestoreInterrupts(OSIntrMode mode)
{
    (void)mode;
}

int OS_Printf(const char *format, ...)
{
    (void)format;
    return 0;
}

void MI_CpuFill32(void *dest, u32 value, u32 size)
{
    u32 *words = dest;

    while (size >= sizeof(*words)) {
        *words++ = value;
        size -= sizeof(*words);
    }
}

BOOL CommManager_IsInitialized(void)
{
    return FALSE;
}

void ErrorMessageReset_PrintErrorAndReset(void)
{
    ErrorHandling_AssertFail();
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

static BOOL IsValidResourcePath(const char *path)
{
    if (path == NULL || path[0] == '\0' || path[0] == '/') {
        return FALSE;
    }

    for (const char *part = path; *part != '\0'; part++) {
        if (*part == '\\' || *part == ':') {
            return FALSE;
        }
        if (part[0] == '.' && part[1] == '.'
            && (part == path || part[-1] == '/')
            && (part[2] == '\0' || part[2] == '/')) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL FS_OpenFile(FSFile *file, const char *path)
{
    char nativePath[512];

    if (!IsValidResourcePath(path)
        || snprintf(nativePath, sizeof(nativePath), "%s%s", sFileRoot, path) >= (int)sizeof(nativePath)) {
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

void RTC_Init(void)
{
    sRTCReadCount = 0;
}

RTCResult RTC_GetDateTimeAsync(RTCDate *date, RTCTime *timeValue, RTCCallback callback, void *arg)
{
    time_t current = time(NULL);
    struct tm *local = localtime(&current);

    if (date == NULL || timeValue == NULL || callback == NULL || local == NULL) {
        return RTC_RESULT_ILLEGAL_PARAMETER;
    }

    int year = local->tm_year + 1900;
    if (year < 2000) {
        year = 2000;
    } else if (year > 2099) {
        year = 2099;
    }

    date->year = (u32)(year - 2000);
    date->month = (u32)(local->tm_mon + 1);
    date->day = (u32)local->tm_mday;
    date->week = (RTCWeek)local->tm_wday;
    timeValue->hour = (u32)local->tm_hour;
    timeValue->minute = (u32)local->tm_min;
    timeValue->second = (u32)local->tm_sec;
    sRTCReadCount++;
    callback(RTC_RESULT_SUCCESS, arg);
    return RTC_RESULT_SUCCESS;
}
