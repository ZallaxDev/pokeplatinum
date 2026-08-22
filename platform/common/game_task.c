#include "platform/game_task.h"

#include <string.h>

bool GameTaskManager_Init(GameTaskManager *manager, size_t capacity)
{
    if (manager == NULL || capacity == 0 || capacity > GAME_TASK_MANAGER_MAX_TASKS) {
        return false;
    }

    memset(manager, 0, sizeof(*manager));
    manager->capacity = capacity;
    manager->sentinel.manager = manager;
    manager->sentinel.previous = &manager->sentinel;
    manager->sentinel.next = &manager->sentinel;
    manager->current = &manager->sentinel;
    manager->next = &manager->sentinel;
    return true;
}

GameTask *GameTaskManager_Add(GameTaskManager *manager, GameTaskCallback callback,
    void *context, uint32_t priority)
{
    GameTask *task = NULL;
    GameTask *iterator;

    if (manager == NULL || callback == NULL || manager->taskCount >= manager->capacity) {
        return NULL;
    }
    for (size_t i = 0; i < manager->capacity; i++) {
        if (!manager->tasks[i].allocated) {
            task = &manager->tasks[i];
            break;
        }
    }
    if (task == NULL) {
        return NULL;
    }

    task->manager = manager;
    task->callback = callback;
    task->context = context;
    task->priority = priority;
    task->allocated = true;
    task->active = !manager->executing || manager->current->priority > priority;

    iterator = manager->sentinel.next;
    while (iterator != &manager->sentinel && iterator->priority <= priority) {
        iterator = iterator->next;
    }
    task->previous = iterator->previous;
    task->next = iterator;
    iterator->previous->next = task;
    iterator->previous = task;
    if (iterator == manager->next) {
        manager->next = task;
    }
    manager->taskCount++;
    return task;
}

bool GameTaskManager_Remove(GameTask *task)
{
    GameTaskManager *manager;

    if (task == NULL || !task->allocated || task->manager == NULL) {
        return false;
    }
    manager = task->manager;
    if (manager->next == task) {
        manager->next = task->next;
    }
    task->previous->next = task->next;
    task->next->previous = task->previous;
    memset(task, 0, sizeof(*task));
    manager->taskCount--;
    return true;
}

bool GameTaskManager_Execute(GameTaskManager *manager)
{
    if (manager == NULL || manager->executing) {
        return false;
    }

    manager->executing = true;
    manager->current = manager->sentinel.next;
    while (manager->current != &manager->sentinel) {
        manager->next = manager->current->next;
        if (manager->current->active) {
            manager->current->callback(manager->current, manager->current->context);
        } else {
            manager->current->active = true;
        }
        manager->current = manager->next;
    }
    manager->current = &manager->sentinel;
    manager->next = &manager->sentinel;
    manager->executing = false;
    return true;
}

size_t GameTaskManager_GetTaskCount(const GameTaskManager *manager)
{
    return manager == NULL ? 0 : manager->taskCount;
}

void *GameTask_GetContext(const GameTask *task)
{
    return task == NULL ? NULL : task->context;
}

uint32_t GameTask_GetPriority(const GameTask *task)
{
    return task == NULL ? 0 : task->priority;
}
