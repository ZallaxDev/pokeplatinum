#include "platform/platform.h"

#include <3ds.h>

#include "platform/debug.h"
#include "platform/graphics.h"
#include "platform/memory.h"
#include "platform/save.h"

static bool sRomfsMounted;
static bool sSaveReady;
static bool sGraphicsReady;

bool Platform_Init(void)
{
    gfxInitDefault();
    sGraphicsReady = PlatformGraphics_Init();
    sRomfsMounted = R_SUCCEEDED(romfsInit());
    Debug_Init();
    PlatformHeap_Init();
    sSaveReady = PlatformSave_Init();

    if (sRomfsMounted) {
        Debug_Log("ROMFS OK");
    } else {
        Debug_Error("ROMFS mount failed");
    }
    if (sGraphicsReady) {
        Debug_Log("GPU OK");
    } else {
        Debug_Error("GPU init failed");
    }
    if (sSaveReady) {
        Debug_Log("SAVE STORAGE OK");
    } else {
        Debug_Error("Save storage init failed");
    }

    return sGraphicsReady && sRomfsMounted && sSaveReady;
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
    PlatformGraphics_Shutdown();
    gfxExit();
}

bool Platform_MainLoop(void)
{
    return aptMainLoop();
}

void Platform_WaitForFrame(void)
{
    if (sGraphicsReady) {
        PlatformGraphics_EndFrame();
    } else {
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
}
