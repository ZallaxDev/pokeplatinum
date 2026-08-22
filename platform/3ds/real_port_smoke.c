#include "real_port_smoke.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants/field/map_tile_behaviors.h"
#include "heap.h"
#include "map_tile_behavior.h"
#include "narc.h"
#include "overlay_manager.h"
#include "runtime_adapters.h"
#include "rtc.h"
#include "sys_task_manager.h"

typedef struct TaskSmokeContext {
    SysTaskManager *manager;
    int order[8];
    int count;
    BOOL addedTask;
} TaskSmokeContext;

typedef struct ApplicationSmokeContext {
    int calls[8];
    int count;
} ApplicationSmokeContext;

static BOOL SmokeApplicationInit(ApplicationManager *appMan, int *state)
{
    ApplicationSmokeContext *context = ApplicationManager_Args(appMan);

    context->calls[context->count++] = 10 + *state;
    if ((*state)++ == 0) {
        return FALSE;
    }

    ApplicationManager_NewData(appMan, sizeof(u32), HEAP_ID_APPLICATION);
    *(u32 *)ApplicationManager_Data(appMan) = 0x12345678;
    return TRUE;
}

static BOOL SmokeApplicationMain(ApplicationManager *appMan, int *state)
{
    ApplicationSmokeContext *context = ApplicationManager_Args(appMan);

    context->calls[context->count++] = 20 + *state;
    return ++(*state) == 2;
}

static BOOL SmokeApplicationExit(ApplicationManager *appMan, int *state)
{
    ApplicationSmokeContext *context = ApplicationManager_Args(appMan);

    context->calls[context->count++] = 30 + *state;
    if (*(u32 *)ApplicationManager_Data(appMan) != 0x12345678) {
        return FALSE;
    }

    ApplicationManager_FreeData(appMan);
    return TRUE;
}

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
    const HeapParam heapTemplates[] = {
        { 2 * 1024 * 1024, OS_ARENA_MAIN },
    };
    const u32 maxTasks = 4;
    void *memory = malloc(SysTaskManager_GetRequiredSize(maxTasks));
    TaskSmokeContext context = { 0 };
    SysTaskManager *manager;
    ApplicationSmokeContext appContext = { 0 };
    const ApplicationManagerTemplate appTemplate = {
        SmokeApplicationInit,
        SmokeApplicationMain,
        SmokeApplicationExit,
        7,
    };
    ApplicationManager *appManager;
    NARC *narc;
    void *narcMember;
    u8 memberMagic[4];
    RTCDate date;
    RTCTime timeValue;
    void *heapLow;
    void *heapHigh;
    void *childAllocation;
    u32 freeBefore;

    Heap_InitSystem(heapTemplates, 1, HEAP_ID_MAX, 0);
    if (!Heap_Create(HEAP_ID_SYSTEM, HEAP_ID_APPLICATION, 512 * 1024)) {
        return Fail(failure, failureSize, "original application heap creation failed");
    }
    freeBefore = HeapExp_FndGetTotalFreeSize(HEAP_ID_APPLICATION);
    heapLow = Heap_Alloc(HEAP_ID_APPLICATION, 64);
    heapHigh = Heap_AllocAtEnd(HEAP_ID_APPLICATION, 128);
    if (heapLow == NULL || heapHigh == NULL
        || ((uintptr_t)heapLow & 3) != 0 || ((uintptr_t)heapHigh & 3) != 0
        || HeapExp_FndGetTotalFreeSize(HEAP_ID_APPLICATION) >= freeBefore) {
        return Fail(failure, failureSize, "original heap allocation failed");
    }
    Heap_Free(heapLow);
    Heap_Free(heapHigh);
    if (!Heap_CreateAtEnd(HEAP_ID_APPLICATION, HEAP_ID_FIELD1, 64 * 1024)) {
        return Fail(failure, failureSize, "original child heap creation failed");
    }
    childAllocation = Heap_Alloc(HEAP_ID_FIELD1, 256);
    if (childAllocation == NULL) {
        return Fail(failure, failureSize, "original child heap allocation failed");
    }
    Heap_Free(childAllocation);
    Heap_Destroy(HEAP_ID_FIELD1);

    FSFile invalidFile;
    FS_InitFile(&invalidFile);
    if (FS_OpenFile(&invalidFile, "/demo/title/titledemo.narc")
        || FS_OpenFile(&invalidFile, "../titledemo.narc")
        || FS_OpenFile(&invalidFile, "demo\\title\\titledemo.narc")) {
        return Fail(failure, failureSize, "native Nitro FS accepted an unsafe path");
    }

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

    RuntimeAdapters_Reset();
    appManager = ApplicationManager_New(&appTemplate, &appContext, HEAP_ID_APPLICATION);
    if (ApplicationManager_Exec(appManager)
        || !RuntimeAdapters_IsOverlayLoaded(7)
        || appContext.count != 1
        || appContext.calls[0] != 10) {
        ApplicationManager_Free(appManager);
        return Fail(failure, failureSize, "original application load/init failed");
    }

    while (!ApplicationManager_Exec(appManager)) {
    }

    if (appContext.count != 5
        || appContext.calls[1] != 11
        || appContext.calls[2] != 20
        || appContext.calls[3] != 21
        || appContext.calls[4] != 30
        || RuntimeAdapters_IsOverlayLoaded(7)
        || RuntimeAdapters_GetOverlayLoadCount() != 1
        || RuntimeAdapters_GetOverlayUnloadCount() != 1) {
        ApplicationManager_Free(appManager);
        return Fail(failure, failureSize, "original application lifecycle failed");
    }
    ApplicationManager_Free(appManager);

    narc = NARC_ctor(NARC_INDEX_DEMO__TITLE__TITLEDEMO, HEAP_ID_APPLICATION);
    if (narc == NULL || NARC_GetFileCount(narc) != 29 || NARC_GetMemberSize(narc, 11) < 4) {
        if (narc != NULL) {
            NARC_dtor(narc);
        }
        return Fail(failure, failureSize, "original NARC metadata parse failed");
    }

    narcMember = NARC_AllocAndReadWholeMember(narc, 11, HEAP_ID_APPLICATION);
    if (narcMember == NULL || memcmp(narcMember, "RLCN", 4) != 0) {
        Heap_Free(narcMember);
        NARC_dtor(narc);
        return Fail(failure, failureSize, "original NARC member read failed");
    }
    Heap_Free(narcMember);

    NARC_ReadFromMember(narc, 11, 0, sizeof(memberMagic), memberMagic);
    if (memcmp(memberMagic, "RLCN", sizeof(memberMagic)) != 0) {
        NARC_dtor(narc);
        return Fail(failure, failureSize, "original partial NARC read failed");
    }
    NARC_dtor(narc);

    NARC_ReadFromMemberByIndexPair(memberMagic, NARC_INDEX_DEMO__TITLE__TITLEDEMO, 11, 0, sizeof(memberMagic));
    if (memcmp(memberMagic, "RLCN", sizeof(memberMagic)) != 0) {
        return Fail(failure, failureSize, "original indexed NARC read failed");
    }

    InitRTC();
    GetCurrentDateTime(&date, &timeValue);
    if (date.year > 99 || date.month < 1 || date.month > 12
        || date.day < 1 || date.day > 31 || date.week >= RTC_WEEK_MAX
        || timeValue.hour > 23 || timeValue.minute > 59 || timeValue.second > 59
        || GetSecondsSinceMidnight() != (int)(timeValue.hour * 3600 + timeValue.minute * 60 + timeValue.second)
        || GetTimestamp() != RTC_ConvertDateTimeToSecond(&date, &timeValue)
        || RuntimeAdapters_GetRTCReadCount() != 1) {
        return Fail(failure, failureSize, "original RTC state initialization failed");
    }

    for (int i = 0; i < 11; i++) {
        UpdateRTC();
    }
    if (RuntimeAdapters_GetRTCReadCount() != 2
        || TimeOfDayForHour(3) != TIMEOFDAY_LATE_NIGHT
        || TimeOfDayForHour(4) != TIMEOFDAY_MORNING
        || TimeOfDayForHour(10) != TIMEOFDAY_DAY
        || TimeOfDayForHour(17) != TIMEOFDAY_TWILIGHT
        || TimeOfDayForHour(20) != TIMEOFDAY_NIGHT
        || DayNumberForDate(&(RTCDate){ 24, 3, 1, RTC_WEEK_FRIDAY }) != 61
        || TimeElapsed(10, 20) != 10) {
        return Fail(failure, failureSize, "original RTC update/calendar logic failed");
    }

    Heap_Destroy(HEAP_ID_APPLICATION);

    failure[0] = '\0';
    return TRUE;
}
