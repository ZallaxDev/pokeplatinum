#include "platform/debug.h"

#include <3ds.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/graphics.h"
#include "platform/platform.h"

static char sLines[DEBUG_LOG_LINES][DEBUG_LOG_LINE_LENGTH];
static unsigned int sNextLine;
static unsigned int sLineCount;
static bool sOverlayEnabled;

static void Debug_Write(const char *level, const char *format, va_list args)
{
    char message[DEBUG_LOG_LINE_LENGTH];
    char external[DEBUG_LOG_LINE_LENGTH + 16];

    vsnprintf(message, sizeof(message), format, args);
    snprintf(sLines[sNextLine], DEBUG_LOG_LINE_LENGTH, "%-5.5s %.89s", level, message);
    snprintf(external, sizeof(external), "[3DS:%s] %s\n", level, message);
    svcOutputDebugString(external, (s32)strlen(external));

    sNextLine = (sNextLine + 1) % DEBUG_LOG_LINES;
    if (sLineCount < DEBUG_LOG_LINES) {
        sLineCount++;
    }
}

void Debug_Init(void)
{
    memset(sLines, 0, sizeof(sLines));
    sNextLine = 0;
    sLineCount = 0;
#if PORT3DS_DEBUG_OVERLAY
    sOverlayEnabled = true;
#else
    sOverlayEnabled = false;
#endif
    Debug_Log("Debug_Init OK");
}

void Debug_Shutdown(void)
{
}

void Debug_Log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    Debug_Write("INFO", format, args);
    va_end(args);
}

void Debug_Warn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    Debug_Write("WARN", format, args);
    va_end(args);
}

void Debug_Error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    Debug_Write("ERROR", format, args);
    va_end(args);
}

_Noreturn void Debug_Fatal(const char *subsystem, const char *function, const char *format, ...)
{
    va_list args;
    char error[DEBUG_LOG_LINE_LENGTH];

    va_start(args, format);
    vsnprintf(error, sizeof(error), format, args);
    va_end(args);
    Debug_Error("FATAL %s/%s: %s", subsystem, function, error);

#if PORT3DS_DEBUG_OVERLAY
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) {
            break;
        }
        PlatformGraphics_BeginFrame();
        PlatformGraphics_BeginScreen(PLATFORM_SCREEN_TOP);
        PlatformGraphics_DrawText(116.0f, 96.0f, 0.65f, PLATFORM_RGBA(255, 100, 100, 255),
            "FATAL ERROR");
        PlatformGraphics_BeginScreen(PLATFORM_SCREEN_BOTTOM);
        PlatformGraphics_DrawText(12.0f, 18.0f, 0.65f, PLATFORM_RGBA(255, 100, 100, 255),
            "FATAL ERROR");
        PlatformGraphics_DrawText(12.0f, 55.0f, 0.45f, PLATFORM_RGBA(255, 255, 255, 255),
            "Subsystem: %s", subsystem);
        PlatformGraphics_DrawText(12.0f, 74.0f, 0.45f, PLATFORM_RGBA(255, 255, 255, 255),
            "Function: %s", function);
        PlatformGraphics_DrawText(12.0f, 93.0f, 0.4f, PLATFORM_RGBA(255, 220, 220, 255),
            "Error: %.80s", error);
        PlatformGraphics_DrawText(12.0f, 190.0f, 0.45f, PLATFORM_RGBA(255, 255, 255, 255),
            "Press START to exit.");
        Platform_WaitForFrame();
    }
#endif

    Platform_Shutdown();
    exit(EXIT_FAILURE);
}

void Debug_SetOverlayEnabled(bool enabled)
{
#if PORT3DS_DEBUG_OVERLAY
    sOverlayEnabled = enabled;
#else
    (void)enabled;
#endif
}

bool Debug_IsOverlayEnabled(void)
{
    return sOverlayEnabled;
}

void Debug_Render(unsigned long long frame, const char *lastInput,
    unsigned long long gameTicks, unsigned long long elapsedNs)
{
#if PORT3DS_DEBUG_OVERLAY
    const unsigned int visibleLines = 14;
    unsigned long long elapsedMs = elapsedNs / 1000000ULL;
    unsigned long long rateMilliHz = elapsedMs == 0 ? 0 : gameTicks * 1000000ULL / elapsedMs;
    unsigned int count;
    unsigned int start;

    if (!sOverlayEnabled) {
        return;
    }

    PlatformGraphics_BeginScreen(PLATFORM_SCREEN_BOTTOM);
    PlatformGraphics_DrawText(8.0f, 5.0f, 0.48f, PLATFORM_RGBA(105, 230, 255, 255),
        "POKEPLATINUM 3DS DEBUG");
    PlatformGraphics_DrawText(8.0f, 22.0f, 0.36f, PLATFORM_RGBA(255, 255, 255, 255),
        "Milestone: BOOT-01  Frame: %llu", frame);
    PlatformGraphics_DrawText(8.0f, 35.0f, 0.36f, PLATFORM_RGBA(255, 255, 255, 255),
        "Last input: %s", lastInput);
    PlatformGraphics_DrawText(8.0f, 48.0f, 0.36f, PLATFORM_RGBA(255, 255, 255, 255),
        "Ticks: %llu  Rate: %llu.%03llu Hz", gameTicks,
        rateMilliHz / 1000ULL, rateMilliHz % 1000ULL);

    count = sLineCount < visibleLines ? sLineCount : visibleLines;
    start = (sNextLine + DEBUG_LOG_LINES - count) % DEBUG_LOG_LINES;
    for (unsigned int i = 0; i < visibleLines; i++) {
        const char *line = i < count ? sLines[(start + i) % DEBUG_LOG_LINES] : "";
        PlatformGraphics_DrawText(8.0f, 68.0f + i * 11.5f, 0.32f,
            PLATFORM_RGBA(240, 225, 235, 255), "%.45s", line);
    }
#else
    (void)frame;
    (void)lastInput;
    (void)gameTicks;
    (void)elapsedNs;
#endif
}
