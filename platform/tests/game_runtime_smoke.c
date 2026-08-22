#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "platform/game_runtime.h"

#define SMOKE_FRAME_COUNT 120

typedef struct SmokeState {
    unsigned int initCount;
    unsigned int frameCount;
    unsigned int shutdownCount;
} SmokeState;

static bool Smoke_Init(void *context)
{
    SmokeState *state = context;
    state->initCount++;
    return true;
}

static bool Smoke_Frame(void *context)
{
    SmokeState *state = context;
    assert(state->initCount == 1);
    assert(state->shutdownCount == 0);
    state->frameCount++;
    return true;
}

static void Smoke_Shutdown(void *context)
{
    SmokeState *state = context;
    assert(state->initCount == 1);
    assert(state->frameCount == SMOKE_FRAME_COUNT);
    state->shutdownCount++;
}

int main(void)
{
    const GameRuntimeHooks hooks = {
        .init = Smoke_Init,
        .frame = Smoke_Frame,
        .shutdown = Smoke_Shutdown,
    };
    SmokeState state = { 0 };
    GameRuntime runtime;

    assert(GameRuntime_Init(&runtime, &hooks, &state));
    for (unsigned int i = 0; i < SMOKE_FRAME_COUNT; i++) {
        assert(GameRuntime_RunFrame(&runtime));
    }
    assert(GameRuntime_GetFrameCount(&runtime) == SMOKE_FRAME_COUNT);
    GameRuntime_Shutdown(&runtime);
    GameRuntime_Shutdown(&runtime);
    assert(state.initCount == 1);
    assert(state.frameCount == SMOKE_FRAME_COUNT);
    assert(state.shutdownCount == 1);
    assert(!GameRuntime_IsRunning(&runtime));
    assert(!GameRuntime_RunFrame(&runtime));

    printf("GAME RUNTIME SMOKE OK: init=1 frames=%u shutdown=1\n", state.frameCount);
    return 0;
}
