#include "platform/graphics.h"

#include <3ds.h>
#include <citro2d.h>
#include <stdarg.h>
#include <stdio.h>

#define GRAPHICS_TEXT_GLYPHS 4096
#define GRAPHICS_TEXT_LENGTH 128

static C3D_RenderTarget *sTopTarget;
static C3D_RenderTarget *sBottomTarget;
static C2D_TextBuf sTextBuffer;
static bool sReady;
static bool sFrameActive;

bool PlatformGraphics_Init(void)
{
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        return false;
    }
    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        C3D_Fini();
        return false;
    }
    C2D_Prepare();

    sTopTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    sBottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    sTextBuffer = C2D_TextBufNew(GRAPHICS_TEXT_GLYPHS);
    if (sTopTarget == NULL || sBottomTarget == NULL || sTextBuffer == NULL) {
        if (sTextBuffer != NULL) {
            C2D_TextBufDelete(sTextBuffer);
        }
        C2D_Fini();
        C3D_Fini();
        return false;
    }

    sReady = true;
    return true;
}

void PlatformGraphics_Shutdown(void)
{
    if (!sReady) {
        return;
    }
    if (sFrameActive) {
        PlatformGraphics_EndFrame();
    }
    C2D_TextBufDelete(sTextBuffer);
    C2D_Fini();
    C3D_Fini();
    sTextBuffer = NULL;
    sTopTarget = NULL;
    sBottomTarget = NULL;
    sReady = false;
}

bool PlatformGraphics_IsReady(void)
{
    return sReady;
}

void PlatformGraphics_BeginFrame(void)
{
    if (!sReady || sFrameActive || !C3D_FrameBegin(C3D_FRAME_SYNCDRAW)) {
        return;
    }

    C2D_TextBufClear(sTextBuffer);
    C2D_TargetClear(sTopTarget, C2D_Color32(8, 28, 52, 255));
    C2D_TargetClear(sBottomTarget, C2D_Color32(50, 18, 42, 255));
    sFrameActive = true;
}

void PlatformGraphics_BeginScreen(PlatformScreen screen)
{
    if (!sFrameActive) {
        return;
    }
    C2D_SceneBegin(screen == PLATFORM_SCREEN_TOP ? sTopTarget : sBottomTarget);
}

void PlatformGraphics_DrawText(float x, float y, float scale, uint32_t color,
    const char *format, ...)
{
    C2D_Text text;
    char buffer[GRAPHICS_TEXT_LENGTH];
    va_list args;

    if (!sFrameActive) {
        return;
    }
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (C2D_TextParse(&text, sTextBuffer, buffer) == NULL) {
        return;
    }
    C2D_DrawText(&text, C2D_WithColor, x, y, 0.0f, scale, scale, color);
}

void PlatformGraphics_EndFrame(void)
{
    if (!sFrameActive) {
        return;
    }
    C3D_FrameEnd(0);
    sFrameActive = false;
}
