#ifndef POKEPLATINUM_PLATFORM_DEBUG_H
#define POKEPLATINUM_PLATFORM_DEBUG_H

#include <stdbool.h>

#define DEBUG_LOG_LINES 64
#define DEBUG_LOG_LINE_LENGTH 96

void Debug_Init(void);
void Debug_Shutdown(void);
void Debug_Log(const char *format, ...);
void Debug_Warn(const char *format, ...);
void Debug_Error(const char *format, ...);
_Noreturn void Debug_Fatal(const char *subsystem, const char *function, const char *format, ...);
void Debug_SetOverlayEnabled(bool enabled);
bool Debug_IsOverlayEnabled(void);
void Debug_Render(unsigned long long frame, const char *lastInput,
    unsigned long long gameTicks, unsigned long long elapsedNs);

#endif
