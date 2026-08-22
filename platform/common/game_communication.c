#include "platform/game_communication.h"

#include <stddef.h>

static const char sUnavailableMessage[] =
    "Communications are unavailable in this port.";

bool GameCommunication_InitOffline(GameCommunication *communication)
{
    if (communication == NULL) {
        return false;
    }
    communication->probeCount = 0;
    communication->initialized = true;
    return true;
}

bool GameCommunication_Probe(GameCommunication *communication,
    GameCommunicationService service, GameCommunicationResult *result)
{
    if (communication == NULL || !communication->initialized || result == NULL
        || service >= GAME_COMMUNICATION_SERVICE_COUNT) {
        return false;
    }
    result->service = service;
    result->status = GAME_COMMUNICATION_STATUS_UNAVAILABLE;
    result->message = sUnavailableMessage;
    communication->probeCount++;
    return true;
}

uint32_t GameCommunication_GetProbeCount(const GameCommunication *communication)
{
    return communication != NULL && communication->initialized
        ? communication->probeCount : 0;
}
