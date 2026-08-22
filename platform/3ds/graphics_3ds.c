#include "platform/graphics.h"

#include <3ds.h>
#include <citro2d.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define GRAPHICS_TEXT_GLYPHS 4096
#define GRAPHICS_TEXT_LENGTH 128
#define LOGICAL_TEXTURE_SIZE 256
#define LOGICAL_SCALE 1.25f
#define TOP_LOGICAL_X 40.0f

typedef struct LogicalSurface {
    C3D_Tex texture;
    Tex3DS_SubTexture subtexture;
    C2D_Image image;
    uint32_t pixels[PLATFORM_LOGICAL_WIDTH * PLATFORM_LOGICAL_HEIGHT];
    bool textureReady;
} LogicalSurface;

static C3D_RenderTarget *sTopTarget;
static C3D_RenderTarget *sBottomTarget;
static C2D_TextBuf sTextBuffer;
static LogicalSurface sLogicalSurfaces[2];
static bool sReady;
static bool sFrameActive;

// PICA textures store each 8x8 tile in Morton order.
static unsigned int MortonOffset(unsigned int x, unsigned int y)
{
    return (x & 1u) | ((y & 1u) << 1)
        | ((x & 2u) << 1) | ((y & 2u) << 2)
        | ((x & 4u) << 2) | ((y & 4u) << 3);
}

static bool InitLogicalSurface(LogicalSurface *surface)
{
    if (!C3D_TexInit(&surface->texture, LOGICAL_TEXTURE_SIZE, LOGICAL_TEXTURE_SIZE, GPU_RGBA8)) {
        return false;
    }
    surface->textureReady = true;
    C3D_TexSetFilter(&surface->texture, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&surface->texture, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    memset(surface->texture.data, 0, surface->texture.size);

    surface->subtexture.width = PLATFORM_LOGICAL_WIDTH;
    surface->subtexture.height = PLATFORM_LOGICAL_HEIGHT;
    surface->subtexture.left = 0.0f;
    surface->subtexture.top = 1.0f;
    surface->subtexture.right = 1.0f;
    surface->subtexture.bottom = 1.0f
        - (float)PLATFORM_LOGICAL_HEIGHT / LOGICAL_TEXTURE_SIZE;
    surface->image.tex = &surface->texture;
    surface->image.subtex = &surface->subtexture;
    return true;
}

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
    if (sTopTarget == NULL || sBottomTarget == NULL || sTextBuffer == NULL
        || !InitLogicalSurface(&sLogicalSurfaces[PLATFORM_SCREEN_TOP])
        || !InitLogicalSurface(&sLogicalSurfaces[PLATFORM_SCREEN_BOTTOM])) {
        for (unsigned int i = 0; i < 2; i++) {
            if (sLogicalSurfaces[i].textureReady) {
                C3D_TexDelete(&sLogicalSurfaces[i].texture);
            }
        }
        if (sTextBuffer != NULL) {
            C2D_TextBufDelete(sTextBuffer);
        }
        C2D_Fini();
        C3D_Fini();
        return false;
    }
    for (unsigned int i = 0; i < 2; i++) {
        PlatformGraphics_UploadLogicalSurface((PlatformScreen)i);
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
    for (unsigned int i = 0; i < 2; i++) {
        C3D_TexDelete(&sLogicalSurfaces[i].texture);
        sLogicalSurfaces[i].textureReady = false;
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

uint32_t *PlatformGraphics_GetLogicalPixels(PlatformScreen screen)
{
    if (screen != PLATFORM_SCREEN_TOP && screen != PLATFORM_SCREEN_BOTTOM) {
        return NULL;
    }
    return sLogicalSurfaces[screen].pixels;
}

bool PlatformGraphics_UploadLogicalSurface(PlatformScreen screen)
{
    LogicalSurface *surface;
    uint32_t *texturePixels;

    if (screen != PLATFORM_SCREEN_TOP && screen != PLATFORM_SCREEN_BOTTOM) {
        return false;
    }
    surface = &sLogicalSurfaces[screen];
    if (!surface->textureReady) {
        return false;
    }
    texturePixels = surface->texture.data;
    for (unsigned int y = 0; y < PLATFORM_LOGICAL_HEIGHT; y++) {
        for (unsigned int x = 0; x < PLATFORM_LOGICAL_WIDTH; x++) {
            unsigned int tile = (y / 8) * (LOGICAL_TEXTURE_SIZE / 8) + x / 8;
            unsigned int offset = tile * 64 + MortonOffset(x & 7, y & 7);
            texturePixels[offset] = surface->pixels[y * PLATFORM_LOGICAL_WIDTH + x];
        }
    }
    C3D_TexFlush(&surface->texture);
    return true;
}

void PlatformGraphics_PresentLogicalSurface(PlatformScreen screen)
{
    float x;

    if (!sFrameActive || (screen != PLATFORM_SCREEN_TOP && screen != PLATFORM_SCREEN_BOTTOM)) {
        return;
    }
    x = screen == PLATFORM_SCREEN_TOP ? TOP_LOGICAL_X : 0.0f;
    PlatformGraphics_BeginScreen(screen);
    C2D_DrawImageAt(sLogicalSurfaces[screen].image, x, 0.0f, 0.0f, NULL,
        LOGICAL_SCALE, LOGICAL_SCALE);
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
