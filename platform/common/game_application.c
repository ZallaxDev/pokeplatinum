#include "platform/game_application.h"

#include <string.h>

bool GameApplicationRegistry_Init(GameApplicationRegistry *registry,
    const GameApplicationTemplate *templates, size_t count)
{
    if (registry == NULL) {
        return false;
    }
    registry->templates = NULL;
    registry->count = 0;
    if (templates == NULL || count == 0) {
        return false;
    }
    for (size_t i = 0; i < count; i++) {
        if (templates[i].id == GAME_APPLICATION_NONE || templates[i].name == NULL
            || templates[i].init == NULL || templates[i].main == NULL
            || templates[i].exit == NULL) {
            return false;
        }
        for (size_t j = i + 1; j < count; j++) {
            if (templates[i].id == templates[j].id) {
                return false;
            }
        }
    }

    registry->templates = templates;
    registry->count = count;
    return true;
}

const GameApplicationTemplate *GameApplicationRegistry_Find(
    const GameApplicationRegistry *registry, uint32_t id)
{
    if (registry == NULL || registry->templates == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < registry->count; i++) {
        if (registry->templates[i].id == id) {
            return &registry->templates[i];
        }
    }
    return NULL;
}

bool GameApplicationRunner_Init(GameApplicationRunner *runner,
    const GameApplicationRegistry *registry, void *context)
{
    if (runner == NULL || registry == NULL || registry->templates == NULL
        || registry->count == 0) {
        return false;
    }
    memset(runner, 0, sizeof(*runner));
    runner->registry = registry;
    runner->context = context;
    runner->pendingId = GAME_APPLICATION_NONE;
    runner->phase = GAME_APPLICATION_PHASE_IDLE;
    runner->initialized = true;
    return true;
}

bool GameApplicationRunner_Queue(GameApplicationRunner *runner, uint32_t id)
{
    if (runner == NULL || !runner->initialized
        || runner->pendingId != GAME_APPLICATION_NONE
        || GameApplicationRegistry_Find(runner->registry, id) == NULL) {
        return false;
    }
    runner->pendingId = id;
    return true;
}

bool GameApplicationRunner_RunFrame(GameApplicationRunner *runner)
{
    bool complete;

    if (runner == NULL || !runner->initialized) {
        return false;
    }
    if (runner->current == NULL) {
        if (runner->pendingId == GAME_APPLICATION_NONE) {
            return true;
        }
        runner->current = GameApplicationRegistry_Find(runner->registry, runner->pendingId);
        if (runner->current == NULL) {
            return false;
        }
        runner->pendingId = GAME_APPLICATION_NONE;
        runner->procedureState = 0;
        runner->phase = GAME_APPLICATION_PHASE_INIT;
    }

    switch (runner->phase) {
    case GAME_APPLICATION_PHASE_INIT:
        complete = runner->current->init(runner, runner->context, &runner->procedureState);
        if (complete) {
            runner->procedureState = 0;
            runner->phase = GAME_APPLICATION_PHASE_MAIN;
        }
        break;
    case GAME_APPLICATION_PHASE_MAIN:
        complete = runner->current->main(runner, runner->context, &runner->procedureState);
        if (complete) {
            runner->procedureState = 0;
            runner->phase = GAME_APPLICATION_PHASE_EXIT;
        }
        break;
    case GAME_APPLICATION_PHASE_EXIT:
        complete = runner->current->exit(runner, runner->context, &runner->procedureState);
        if (complete) {
            runner->current = NULL;
            runner->procedureState = 0;
            runner->phase = GAME_APPLICATION_PHASE_IDLE;
        }
        break;
    case GAME_APPLICATION_PHASE_IDLE:
    default:
        return false;
    }
    return true;
}

uint32_t GameApplicationRunner_GetCurrentId(const GameApplicationRunner *runner)
{
    return runner == NULL || runner->current == NULL
        ? GAME_APPLICATION_NONE
        : runner->current->id;
}

const char *GameApplicationRunner_GetCurrentName(const GameApplicationRunner *runner)
{
    return runner == NULL || runner->current == NULL ? "NONE" : runner->current->name;
}

GameApplicationPhase GameApplicationRunner_GetPhase(const GameApplicationRunner *runner)
{
    return runner == NULL ? GAME_APPLICATION_PHASE_IDLE : runner->phase;
}
