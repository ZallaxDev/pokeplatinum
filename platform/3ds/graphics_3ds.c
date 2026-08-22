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
#define SPRITE_TEXTURE_SIZE 32

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
static C3D_Tex sSpriteTexture;
static Tex3DS_SubTexture sSpriteSubtexture;
static C2D_Image sSpriteImage;
static bool sSpriteTextureReady;
static bool sReady;
static bool sFrameActive;

// PICA textures store each 8x8 tile in Morton order.
static unsigned int MortonOffset(unsigned int x, unsigned int y)
{
    return (x & 1u) | ((y & 1u) << 1)
        | ((x & 2u) << 1) | ((y & 2u) << 2)
        | ((x & 4u) << 2) | ((y & 4u) << 3);
}

static unsigned int TiledOffset(unsigned int textureWidth, unsigned int x, unsigned int y)
{
    unsigned int tile = (y / 8) * (textureWidth / 8) + x / 8;
    return tile * 64 + MortonOffset(x & 7, y & 7);
}

static uint32_t TextureColor(uint32_t rgba)
{
    return (rgba & 0x000000FFu) << 24 | (rgba & 0x0000FF00u) << 8
        | (rgba & 0x00FF0000u) >> 8 | (rgba & 0xFF000000u) >> 24;
}

static bool InitSpriteTexture(void)
{
    uint32_t *pixels;

    if (!C3D_TexInit(&sSpriteTexture, SPRITE_TEXTURE_SIZE, SPRITE_TEXTURE_SIZE, GPU_RGBA8)) {
        return false;
    }
    sSpriteTextureReady = true;
    C3D_TexSetFilter(&sSpriteTexture, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&sSpriteTexture, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    pixels = sSpriteTexture.data;
    for (unsigned int y = 0; y < SPRITE_TEXTURE_SIZE; y++) {
        for (unsigned int x = 0; x < SPRITE_TEXTURE_SIZE; x++) {
            unsigned int distance = (x > 15 ? x - 15 : 15 - x)
                + (y > 15 ? y - 15 : 15 - y);
            uint32_t color = PLATFORM_RGBA(0, 0, 0, 0);

            if (distance <= 14) {
                bool checker = ((x / 4) + (y / 4)) & 1;
                color = checker
                    ? PLATFORM_RGBA(255, 208, 70, 255)
                    : PLATFORM_RGBA(70, 210, 255, 255);
                if (distance >= 12) {
                    color = PLATFORM_RGBA(255, 90, 180, 255);
                }
            }
            pixels[TiledOffset(SPRITE_TEXTURE_SIZE, x, y)] = TextureColor(color);
        }
    }
    C3D_TexFlush(&sSpriteTexture);

    sSpriteSubtexture = (Tex3DS_SubTexture) {
        .width = SPRITE_TEXTURE_SIZE,
        .height = SPRITE_TEXTURE_SIZE,
        .left = 0.0f,
        .top = 1.0f,
        .right = 1.0f,
        .bottom = 0.0f,
    };
    sSpriteImage = (C2D_Image) {
        .tex = &sSpriteTexture,
        .subtex = &sSpriteSubtexture,
    };
    return true;
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
        || !InitLogicalSurface(&sLogicalSurfaces[PLATFORM_SCREEN_BOTTOM])
        || !InitSpriteTexture()) {
        if (sSpriteTextureReady) {
            C3D_TexDelete(&sSpriteTexture);
            sSpriteTextureReady = false;
        }
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
    C3D_TexDelete(&sSpriteTexture);
    sSpriteTextureReady = false;
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
            unsigned int offset = TiledOffset(LOGICAL_TEXTURE_SIZE, x, y);
            texturePixels[offset] = TextureColor(
                surface->pixels[y * PLATFORM_LOGICAL_WIDTH + x]);
        }
    }
    C3D_TexFlush(&surface->texture);
    return true;
}

void PlatformGraphics_DrawSpriteTest(void)
{
    C2D_ImageTint alphaTint;

    if (!sFrameActive || !sSpriteTextureReady) {
        return;
    }
    PlatformGraphics_BeginScreen(PLATFORM_SCREEN_TOP);
    C2D_DrawImageAt(sSpriteImage, 70.0f, 174.0f, 0.0f, NULL, 1.0f, 1.0f);
    C2D_DrawImageAtRotated(sSpriteImage, 151.0f, 190.0f, 0.0f, 0.35f,
        NULL, 0.8f, 0.8f);
    C2D_DrawImageAtRotated(sSpriteImage, 236.0f, 188.0f, 0.0f, -0.22f,
        NULL, 1.15f, 1.15f);
    C2D_DrawRectSolid(292.0f, 173.0f, 0.0f, 40.0f, 36.0f,
        C2D_Color32(235, 245, 255, 255));
    C2D_AlphaImageTint(&alphaTint, 0.45f);
    C2D_DrawImageAt(sSpriteImage, 296.0f, 175.0f, 0.0f, &alphaTint, 1.0f, 1.0f);
}

bool PlatformGraphics_SetSpriteTexture(const uint32_t *pixels, unsigned int width,
    unsigned int height)
{
    uint32_t *texturePixels;

    if (!sSpriteTextureReady || pixels == NULL
        || width != SPRITE_TEXTURE_SIZE || height != SPRITE_TEXTURE_SIZE) {
        return false;
    }
    texturePixels = sSpriteTexture.data;
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            texturePixels[TiledOffset(SPRITE_TEXTURE_SIZE, x, y)] = TextureColor(
                pixels[y * width + x]);
        }
    }
    C3D_TexFlush(&sSpriteTexture);
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
