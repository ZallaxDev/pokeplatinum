#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "platform/game_task.h"

typedef struct SmokeState {
    GameTaskManager *manager;
    unsigned int order[8];
    unsigned int orderCount;
    unsigned int deferredCount;
} SmokeState;

static void RecordTask(GameTask *task, void *context)
{
    SmokeState *state = context;
    state->order[state->orderCount++] = GameTask_GetPriority(task);
}

static void DeferredTask(GameTask *task, void *context)
{
    SmokeState *state = context;
    (void)task;
    state->deferredCount++;
}

static void AddDuringCallback(GameTask *task, void *context)
{
    SmokeState *state = context;
    RecordTask(task, context);
    assert(GameTaskManager_Add(state->manager, DeferredTask, state, 30) != NULL);
    assert(GameTaskManager_Remove(task));
}

int main(void)
{
    GameTaskManager manager;
    SmokeState state = {
        .manager = &manager,
    };

    assert(GameTaskManager_Init(&manager, 4));
    assert(GameTaskManager_Add(&manager, RecordTask, &state, 20) != NULL);
    assert(GameTaskManager_Add(&manager, AddDuringCallback, &state, 10) != NULL);
    assert(GameTaskManager_Execute(&manager));
    assert(state.orderCount == 2);
    assert(state.order[0] == 10 && state.order[1] == 20);
    assert(state.deferredCount == 0);
    assert(GameTaskManager_GetTaskCount(&manager) == 2);

    assert(GameTaskManager_Execute(&manager));
    assert(state.orderCount == 3 && state.order[2] == 20);
    assert(state.deferredCount == 1);

    printf("GAME TASK SMOKE OK: priority=10,20 deferred=1 self-delete=1\n");
    return 0;
}
