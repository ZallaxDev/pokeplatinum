#ifndef POKEPLATINUM_3DS_RUNTIME_ADAPTERS_H
#define POKEPLATINUM_3DS_RUNTIME_ADAPTERS_H

#include "nitro/fs.h"

void RuntimeAdapters_Reset(void);
BOOL RuntimeAdapters_IsOverlayLoaded(FSOverlayID overlayID);
u32 RuntimeAdapters_GetOverlayLoadCount(void);
u32 RuntimeAdapters_GetOverlayUnloadCount(void);

#endif // POKEPLATINUM_3DS_RUNTIME_ADAPTERS_H
