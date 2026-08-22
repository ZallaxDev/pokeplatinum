#ifndef POKEPLATINUM_PLATFORM_GAME_TASK_H
#define POKEPLATINUM_PLATFORM_GAME_TASK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GAME_TASK_MANAGER_MAX_TASKS 32

typedef struct GameTask GameTask;
typedef struct GameTaskManager GameTaskManager;
typedef void (*GameTaskCallback)(GameTask *task, void *context);

struct GameTask {
    GameTaskManager *manager;
    GameTask *previous;
    GameTask *next;
    GameTaskCallback callback;
    void *context;
    uint32_t priority;
    bool allocated;
    bool active;
};

struct GameTaskManager {
    GameTask tasks[GAME_TASK_MANAGER_MAX_TASKS];
    GameTask sentinel;
    GameTask *current;
    GameTask *next;
    size_t capacity;
    size_t taskCount;
    bool executing;
};

bool GameTaskManager_Init(GameTaskManager *manager, size_t capacity);
GameTask *GameTaskManager_Add(GameTaskManager *manager, GameTaskCallback callback,
    void *context, uint32_t priority);
bool GameTaskManager_Remove(GameTask *task);
bool GameTaskManager_Execute(GameTaskManager *manager);
size_t GameTaskManager_GetTaskCount(const GameTaskManager *manager);
void *GameTask_GetContext(const GameTask *task);
uint32_t GameTask_GetPriority(const GameTask *task);

#endif
