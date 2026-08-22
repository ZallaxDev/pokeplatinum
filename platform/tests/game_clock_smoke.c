#include "platform/game_clock.h"

#include <stdio.h>

static int CheckRollover(GameClockDateTime before,
    uint16_t year, uint8_t month, uint8_t day)
{
    GameClockDateTime after;
    uint64_t seconds;

    return GameClock_ToSeconds(&before, &seconds)
        && GameClock_FromSeconds(seconds + 1, &after)
        && after.year == year && after.month == month && after.day == day
        && after.hour == 0 && after.minute == 0 && after.second == 0;
}

int main(void)
{
    GameClockDateTime leapDay = { 2024, 2, 29, 0, 12, 0, 0 };

    if (!GameClock_IsLeapYear(2000) || !GameClock_IsLeapYear(2024)
        || GameClock_IsLeapYear(2100)
        || GameClock_DayOfYear(&leapDay) != 60
        || !CheckRollover((GameClockDateTime){ 2024, 2, 28, 0, 23, 59, 59 },
            2024, 2, 29)
        || !CheckRollover((GameClockDateTime){ 2024, 2, 29, 0, 23, 59, 59 },
            2024, 3, 1)
        || !CheckRollover((GameClockDateTime){ 2025, 12, 31, 0, 23, 59, 59 },
            2026, 1, 1)) {
        fprintf(stderr, "GAME CLOCK SMOKE FAILED\n");
        return 1;
    }
    printf("GAME CLOCK SMOKE OK: leap day and year rollover\n");
    return 0;
}
