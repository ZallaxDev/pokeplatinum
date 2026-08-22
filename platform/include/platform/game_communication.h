#ifndef POKEPLATINUM_PLATFORM_GAME_COMMUNICATION_H
#define POKEPLATINUM_PLATFORM_GAME_COMMUNICATION_H

#include <stdbool.h>
#include <stdint.h>

typedef enum GameCommunicationService {
    GAME_COMMUNICATION_LOCAL_WIRELESS,
    GAME_COMMUNICATION_NINTENDO_WFC,
    GAME_COMMUNICATION_GTS,
    GAME_COMMUNICATION_MYSTERY_GIFT,
    GAME_COMMUNICATION_SERVICE_COUNT,
} GameCommunicationService;

typedef enum GameCommunicationStatus {
    GAME_COMMUNICATION_STATUS_UNAVAILABLE,
} GameCommunicationStatus;

typedef struct GameCommunicationResult {
    GameCommunicationService service;
    GameCommunicationStatus status;
    const char *message;
} GameCommunicationResult;

typedef struct GameCommunication {
    uint32_t probeCount;
    bool initialized;
} GameCommunication;

bool GameCommunication_InitOffline(GameCommunication *communication);
bool GameCommunication_Probe(GameCommunication *communication,
    GameCommunicationService service, GameCommunicationResult *result);
uint32_t GameCommunication_GetProbeCount(const GameCommunication *communication);

#endif
