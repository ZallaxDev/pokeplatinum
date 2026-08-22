#ifndef POKEPLATINUM_PLATFORM_GAME_APPLICATION_H
#define POKEPLATINUM_PLATFORM_GAME_APPLICATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GAME_APPLICATION_NONE UINT32_MAX

typedef struct GameApplicationRunner GameApplicationRunner;
typedef bool (*GameApplicationCallback)(GameApplicationRunner *runner,
    void *context, int *state);

typedef struct GameApplicationTemplate {
    uint32_t id;
    const char *name;
    GameApplicationCallback init;
    GameApplicationCallback main;
    GameApplicationCallback exit;
} GameApplicationTemplate;

typedef struct GameApplicationRegistry {
    const GameApplicationTemplate *templates;
    size_t count;
} GameApplicationRegistry;

typedef enum GameApplicationPhase {
    GAME_APPLICATION_PHASE_IDLE,
    GAME_APPLICATION_PHASE_INIT,
    GAME_APPLICATION_PHASE_MAIN,
    GAME_APPLICATION_PHASE_EXIT,
} GameApplicationPhase;

struct GameApplicationRunner {
    const GameApplicationRegistry *registry;
    const GameApplicationTemplate *current;
    void *context;
    uint32_t pendingId;
    int procedureState;
    GameApplicationPhase phase;
    bool initialized;
};

bool GameApplicationRegistry_Init(GameApplicationRegistry *registry,
    const GameApplicationTemplate *templates, size_t count);
const GameApplicationTemplate *GameApplicationRegistry_Find(
    const GameApplicationRegistry *registry, uint32_t id);
bool GameApplicationRunner_Init(GameApplicationRunner *runner,
    const GameApplicationRegistry *registry, void *context);
bool GameApplicationRunner_Queue(GameApplicationRunner *runner, uint32_t id);
bool GameApplicationRunner_RunFrame(GameApplicationRunner *runner);
uint32_t GameApplicationRunner_GetCurrentId(const GameApplicationRunner *runner);
const char *GameApplicationRunner_GetCurrentName(const GameApplicationRunner *runner);
GameApplicationPhase GameApplicationRunner_GetPhase(const GameApplicationRunner *runner);

#endif
