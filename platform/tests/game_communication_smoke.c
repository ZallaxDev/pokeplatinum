#include "platform/game_communication.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    GameCommunication communication;
    GameCommunicationResult result;

    if (!GameCommunication_InitOffline(&communication)) {
        fprintf(stderr, "GAME COMMUNICATION SMOKE FAILED: init\n");
        return 1;
    }
    for (int service = 0; service < GAME_COMMUNICATION_SERVICE_COUNT; service++) {
        if (!GameCommunication_Probe(&communication,
                (GameCommunicationService)service, &result)
            || result.service != (GameCommunicationService)service
            || result.status != GAME_COMMUNICATION_STATUS_UNAVAILABLE
            || result.message == NULL
            || strcmp(result.message,
                "Communications are unavailable in this port.") != 0) {
            fprintf(stderr, "GAME COMMUNICATION SMOKE FAILED: service %d\n", service);
            return 1;
        }
    }
    if (GameCommunication_GetProbeCount(&communication)
        != GAME_COMMUNICATION_SERVICE_COUNT) {
        fprintf(stderr, "GAME COMMUNICATION SMOKE FAILED: probe count\n");
        return 1;
    }
    printf("GAME COMMUNICATION SMOKE OK: 4 immediate unavailable results\n");
    return 0;
}
