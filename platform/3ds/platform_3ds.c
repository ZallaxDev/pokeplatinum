#include "platform/platform.h"

#include <3ds.h>

#include "platform/debug.h"
#include "platform/memory.h"
#include "platform/save.h"

static bool sRomfsMounted;
static bool sSaveReady;

bool Platform_Init(void)
{
    gfxInitDefault();
    sRomfsMounted = R_SUCCEEDED(romfsInit());
    Debug_Init();
    PlatformHeap_Init();
    sSaveReady = PlatformSave_Init();

    if (sRomfsMounted) {
        Debug_Log("ROMFS OK");
    } else {
        Debug_Error("ROMFS mount failed");
    }
    if (sSaveReady) {
        Debug_Log("SAVE STORAGE OK");
    } else {
        Debug_Error("Save storage init failed");
    }

    return sRomfsMounted && sSaveReady;
}

void Platform_Shutdown(void)
{
    PlatformHeap_Shutdown();
    if (sSaveReady) {
        PlatformSave_Shutdown();
    }
    Debug_Shutdown();
    if (sRomfsMounted) {
        romfsExit();
    }
    gfxExit();
}

bool Platform_MainLoop(void)
{
    return aptMainLoop();
}

void Platform_WaitForFrame(void)
{
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}
