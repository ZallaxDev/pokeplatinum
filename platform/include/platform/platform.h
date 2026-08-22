#ifndef POKEPLATINUM_PLATFORM_PLATFORM_H
#define POKEPLATINUM_PLATFORM_PLATFORM_H

#include <stdbool.h>

bool Platform_Init(void);
void Platform_Shutdown(void);
bool Platform_MainLoop(void);
void Platform_WaitForFrame(void);

#endif
