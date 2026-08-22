#include "platform/game_clock.h"

#include <3ds.h>

#define MILLISECONDS_1900_TO_2000 3155673600000ULL

bool GameClock_Read(GameClockDateTime *dateTime)
{
    uint64_t milliseconds = osGetTime();

    if (milliseconds < MILLISECONDS_1900_TO_2000) {
        return false;
    }
    return GameClock_FromSeconds(
        (milliseconds - MILLISECONDS_1900_TO_2000) / 1000, dateTime);
}
