#include "platform/platform.h"

#include <3ds.h>

#include "platform/debug.h"

static bool sRomfsMounted;

bool Platform_Init(void)
{
    gfxInitDefault();
    sRomfsMounted = R_SUCCEEDED(romfsInit());
    Debug_Init();

    if (sRomfsMounted) {
        Debug_Log("ROMFS OK");
    } else {
        Debug_Error("ROMFS mount failed");
    }

    return sRomfsMounted;
}

void Platform_Shutdown(void)
{
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
