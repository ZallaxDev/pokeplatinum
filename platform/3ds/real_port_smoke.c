#include "real_port_smoke.h"

#include <stdio.h>
#include <stdlib.h>

#include "constants/field/map_tile_behaviors.h"
#include "map_tile_behavior.h"
#include "sys_task_manager.h"

typedef struct TaskSmokeContext {
    SysTaskManager *manager;
    int order[8];
    int count;
    BOOL addedTask;
} TaskSmokeContext;

static void RecordTask(SysTask *task, void *param)
{
    TaskSmokeContext *context = param;

    context->order[context->count++] = (int)SysTask_GetPriority(task);
}

static void AddDeferredTask(SysTask *task, void *param)
{
    TaskSmokeContext *context = param;

    context->order[context->count++] = (int)SysTask_GetPriority(task);
    if (!context->addedTask) {
        context->addedTask = TRUE;
        SysTaskManager_AddTask(context->manager, RecordTask, context, 20);
    }
}

static void DeleteSelfTask(SysTask *task, void *param)
{
    TaskSmokeContext *context = param;

    context->order[context->count++] = (int)SysTask_GetPriority(task);
    SysTask_Delete(task);
}

static BOOL Fail(char *failure, size_t failureSize, const char *message)
{
    snprintf(failure, failureSize, "%s", message);
    return FALSE;
}

BOOL RealPortSmoke_Run(char *failure, size_t failureSize)
{
    const u32 maxTasks = 4;
    void *memory = malloc(SysTaskManager_GetRequiredSize(maxTasks));
    TaskSmokeContext context = { 0 };
    SysTaskManager *manager;

    if (memory == NULL) {
        return Fail(failure, failureSize, "scheduler allocation failed");
    }

    manager = SysTaskManager_Init(maxTasks, memory);
    context.manager = manager;
    SysTaskManager_AddTask(manager, RecordTask, &context, 20);
    SysTaskManager_AddTask(manager, RecordTask, &context, 10);
    SysTaskManager_ExecuteTasks(manager);
    if (context.count != 2 || context.order[0] != 10 || context.order[1] != 20) {
        free(memory);
        return Fail(failure, failureSize, "original scheduler priority order failed");
    }

    SysTaskManager_InternalInit(manager);
    context.count = 0;
    context.addedTask = FALSE;
    SysTaskManager_AddTask(manager, AddDeferredTask, &context, 10);
    SysTaskManager_ExecuteTasks(manager);
    if (context.count != 1 || context.order[0] != 10) {
        free(memory);
        return Fail(failure, failureSize, "new task ran in its creation frame");
    }

    SysTaskManager_ExecuteTasks(manager);
    if (context.count != 3 || context.order[1] != 10 || context.order[2] != 20) {
        free(memory);
        return Fail(failure, failureSize, "deferred original task did not run");
    }

    SysTaskManager_InternalInit(manager);
    context.count = 0;
    SysTaskManager_AddTask(manager, DeleteSelfTask, &context, 5);
    SysTaskManager_ExecuteTasks(manager);
    SysTaskManager_ExecuteTasks(manager);
    if (context.count != 1 || context.order[0] != 5) {
        free(memory);
        return Fail(failure, failureSize, "self-deleting original task ran again");
    }

    free(memory);

    if (!TileBehavior_IsTallGrass(TILE_BEHAVIOR_TALL_GRASS)
        || !TileBehavior_HasEncounters(TILE_BEHAVIOR_TALL_GRASS)
        || !TileBehavior_IsSurfable(TILE_BEHAVIOR_WATER_RIVER)
        || !TileBehavior_IsDoor(TILE_BEHAVIOR_DOOR)
        || !TileBehavior_BlocksMovementNorthward(TILE_BEHAVIOR_BLOCK_NORTHWARD)
        || TileBehavior_IsSurfable(TILE_BEHAVIOR_SAND)) {
        return Fail(failure, failureSize, "original tile behavior rules failed");
    }

    failure[0] = '\0';
    return TRUE;
}
