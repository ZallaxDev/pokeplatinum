#ifndef POKEPLATINUM_PLATFORM_GAME_CLOCK_H
#define POKEPLATINUM_PLATFORM_GAME_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

typedef struct GameClockDateTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekDay;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} GameClockDateTime;

bool GameClock_IsLeapYear(uint16_t year);
int GameClock_DayOfYear(const GameClockDateTime *dateTime);
bool GameClock_ToSeconds(const GameClockDateTime *dateTime, uint64_t *seconds);
bool GameClock_FromSeconds(uint64_t seconds, GameClockDateTime *dateTime);
bool GameClock_Read(GameClockDateTime *dateTime);

#endif
