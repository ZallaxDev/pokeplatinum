#ifndef POKEPLATINUM_PLATFORM_GAME_RUNTIME_H
#define POKEPLATINUM_PLATFORM_GAME_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

typedef struct GameRuntimeHooks {
    bool (*init)(void *context);
    bool (*frame)(void *context);
    void (*shutdown)(void *context);
} GameRuntimeHooks;

typedef struct GameRuntime {
    GameRuntimeHooks hooks;
    void *context;
    uint64_t frameCount;
    bool initialized;
    bool shutdownCalled;
} GameRuntime;

bool GameRuntime_Init(GameRuntime *runtime, const GameRuntimeHooks *hooks, void *context);
bool GameRuntime_RunFrame(GameRuntime *runtime);
void GameRuntime_Shutdown(GameRuntime *runtime);
uint64_t GameRuntime_GetFrameCount(const GameRuntime *runtime);
bool GameRuntime_IsRunning(const GameRuntime *runtime);

#endif
