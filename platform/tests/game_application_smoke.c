#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "platform/game_application.h"

enum {
    APP_ALPHA = 1,
    APP_BETA = 2,
};

typedef struct SmokeState {
    unsigned int events[6];
    unsigned int eventCount;
} SmokeState;

static void RecordEvent(GameApplicationRunner *runner, SmokeState *state, unsigned int phase)
{
    uint32_t id = GameApplicationRunner_GetCurrentId(runner);
    assert(state->eventCount < 6);
    state->events[state->eventCount++] = id * 10 + phase;
}

static bool Smoke_Init(GameApplicationRunner *runner, void *context, int *stateValue)
{
    SmokeState *state = context;
    (void)stateValue;
    RecordEvent(runner, state, 1);
    return true;
}

static bool Smoke_Main(GameApplicationRunner *runner, void *context, int *stateValue)
{
    SmokeState *state = context;
    if (*stateValue == 0) {
        RecordEvent(runner, state, 2);
    }
    (*stateValue)++;
    return *stateValue == 2;
}

static bool Smoke_Exit(GameApplicationRunner *runner, void *context, int *stateValue)
{
    SmokeState *state = context;
    uint32_t id = GameApplicationRunner_GetCurrentId(runner);
    (void)stateValue;
    RecordEvent(runner, state, 3);
    if (id == APP_ALPHA) {
        assert(GameApplicationRunner_Queue(runner, APP_BETA));
    }
    return true;
}

int main(void)
{
    const GameApplicationTemplate templates[] = {
        { APP_ALPHA, "ALPHA", Smoke_Init, Smoke_Main, Smoke_Exit },
        { APP_BETA, "BETA", Smoke_Init, Smoke_Main, Smoke_Exit },
    };
    const unsigned int expected[] = { 11, 12, 13, 21, 22, 23 };
    GameApplicationRegistry registry;
    GameApplicationRunner runner;
    SmokeState state = { 0 };

    assert(GameApplicationRegistry_Init(&registry, templates, 2));
    assert(GameApplicationRunner_Init(&runner, &registry, &state));
    assert(GameApplicationRunner_Queue(&runner, APP_ALPHA));
    for (unsigned int i = 0; i < 8; i++) {
        assert(GameApplicationRunner_RunFrame(&runner));
    }
    assert(state.eventCount == 6);
    for (unsigned int i = 0; i < 6; i++) {
        assert(state.events[i] == expected[i]);
    }
    assert(GameApplicationRunner_GetCurrentId(&runner) == GAME_APPLICATION_NONE);

    printf("GAME APPLICATION SMOKE OK: A(init/main/exit) -> B(init/main/exit)\n");
    return 0;
}
