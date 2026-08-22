#ifndef POKEPLATINUM_PLATFORM_GRAPHICS_H
#define POKEPLATINUM_PLATFORM_GRAPHICS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum PlatformScreen {
    PLATFORM_SCREEN_TOP,
    PLATFORM_SCREEN_BOTTOM,
} PlatformScreen;

#define PLATFORM_LOGICAL_WIDTH 256
#define PLATFORM_LOGICAL_HEIGHT 192

#define PLATFORM_RGBA(r, g, b, a) \
    ((uint32_t)(r) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24))

bool PlatformGraphics_Init(void);
void PlatformGraphics_Shutdown(void);
bool PlatformGraphics_IsReady(void);
void PlatformGraphics_BeginFrame(void);
void PlatformGraphics_BeginScreen(PlatformScreen screen);
uint32_t *PlatformGraphics_GetLogicalPixels(PlatformScreen screen);
bool PlatformGraphics_UploadLogicalSurface(PlatformScreen screen);
void PlatformGraphics_PresentLogicalSurface(PlatformScreen screen);
void PlatformGraphics_DrawSpriteTest(void);
void PlatformGraphics_DrawText(float x, float y, float scale, uint32_t color,
    const char *format, ...);
void PlatformGraphics_EndFrame(void);

#endif
