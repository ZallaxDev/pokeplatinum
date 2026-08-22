#include "platform/debug.h"

#include <3ds.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/platform.h"

static char sLines[DEBUG_LOG_LINES][DEBUG_LOG_LINE_LENGTH];
static unsigned int sNextLine;
static unsigned int sLineCount;
static bool sOverlayEnabled;

#if PORT3DS_DEBUG_OVERLAY
static PrintConsole sBottomConsole;
#endif

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
    consoleInit(GFX_BOTTOM, &sBottomConsole);
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
    consoleSelect(&sBottomConsole);
    consoleClear();
    printf("\x1b[31;1mFATAL ERROR\x1b[0m\n\n");
    printf("Subsystem: %s\nFunction: %s\nError: %s\n\n", subsystem, function, error);
    printf("Press START to exit.\n");
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) {
            break;
        }
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
#endif

    Platform_Shutdown();
    exit(EXIT_FAILURE);
}

void Debug_SetOverlayEnabled(bool enabled)
{
#if PORT3DS_DEBUG_OVERLAY
    if (sOverlayEnabled != enabled) {
        consoleSelect(&sBottomConsole);
        consoleClear();
        sOverlayEnabled = enabled;
    }
#else
    (void)enabled;
#endif
}

bool Debug_IsOverlayEnabled(void)
{
    return sOverlayEnabled;
}

void Debug_Render(unsigned long long frame, const char *lastInput)
{
#if PORT3DS_DEBUG_OVERLAY
    const unsigned int visibleLines = 18;
    unsigned int count;
    unsigned int start;

    if (!sOverlayEnabled) {
        return;
    }

    consoleSelect(&sBottomConsole);
    printf("\x1b[1;1H\x1b[36;1mPOKEPLATINUM 3DS DEBUG\x1b[0m");
    printf("\x1b[2;1HMilestone: BOOT-01  Frame: %-10llu", frame);
    printf("\x1b[3;1HLast input: %-24s", lastInput);

    count = sLineCount < visibleLines ? sLineCount : visibleLines;
    start = (sNextLine + DEBUG_LOG_LINES - count) % DEBUG_LOG_LINES;
    for (unsigned int i = 0; i < visibleLines; i++) {
        const char *line = i < count ? sLines[(start + i) % DEBUG_LOG_LINES] : "";
        printf("\x1b[%u;1H%-40.40s", i + 5, line);
    }
#else
    (void)frame;
    (void)lastInput;
#endif
}
