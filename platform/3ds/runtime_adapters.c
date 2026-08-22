#include "runtime_adapters.h"

#include <stdlib.h>
#include <string.h>

#include "game_overlay.h"
#include "heap.h"

#define STATIC_OVERLAY_COUNT 128

static BOOL sLoadedOverlays[STATIC_OVERLAY_COUNT];
static u32 sOverlayLoadCount;
static u32 sOverlayUnloadCount;

void RuntimeAdapters_Reset(void)
{
    memset(sLoadedOverlays, 0, sizeof(sLoadedOverlays));
    sOverlayLoadCount = 0;
    sOverlayUnloadCount = 0;
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
