#include "platform/game_runtime.h"

#include <string.h>

bool GameRuntime_Init(GameRuntime *runtime, const GameRuntimeHooks *hooks, void *context)
{
    if (runtime == NULL || hooks == NULL || hooks->init == NULL
        || hooks->frame == NULL || hooks->shutdown == NULL) {
        return false;
    }

    memset(runtime, 0, sizeof(*runtime));
    runtime->hooks = *hooks;
    runtime->context = context;
    if (!runtime->hooks.init(runtime->context)) {
        memset(runtime, 0, sizeof(*runtime));
        return false;
    }
    runtime->initialized = true;
    return true;
}

bool GameRuntime_RunFrame(GameRuntime *runtime)
{
    if (!GameRuntime_IsRunning(runtime) || !runtime->hooks.frame(runtime->context)) {
        return false;
    }
    runtime->frameCount++;
    return true;
}

void GameRuntime_Shutdown(GameRuntime *runtime)
{
    if (!GameRuntime_IsRunning(runtime)) {
        return;
    }
    runtime->shutdownCalled = true;
    runtime->hooks.shutdown(runtime->context);
}

uint64_t GameRuntime_GetFrameCount(const GameRuntime *runtime)
{
    return runtime == NULL ? 0 : runtime->frameCount;
}

bool GameRuntime_IsRunning(const GameRuntime *runtime)
{
    return runtime != NULL && runtime->initialized && !runtime->shutdownCalled;
}
