#include "platform/game_clock.h"

#include <stddef.h>

#define GAME_CLOCK_FIRST_YEAR 2000
#define GAME_CLOCK_LAST_YEAR 2099
#define GAME_CLOCK_DAYS_PER_CYCLE 36525
#define GAME_CLOCK_SECONDS_PER_DAY 86400

static const uint16_t sMonthStart[] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
};

bool GameClock_IsLeapYear(uint16_t year)
{
    return ((year % 4) == 0 && (year % 100) != 0) || (year % 400) == 0;
}

static uint8_t GameClock_DaysInMonth(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
    };

    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && GameClock_IsLeapYear(year)) {
        return 29;
    }
    return days[month - 1];
}

int GameClock_DayOfYear(const GameClockDateTime *dateTime)
{
    int day;

    if (dateTime == NULL || dateTime->year < GAME_CLOCK_FIRST_YEAR
        || dateTime->year > GAME_CLOCK_LAST_YEAR || dateTime->month < 1
        || dateTime->month > 12
        || dateTime->day < 1
        || dateTime->day > GameClock_DaysInMonth(dateTime->year, dateTime->month)) {
        return 0;
    }
    day = sMonthStart[dateTime->month - 1] + dateTime->day;
    if (dateTime->month >= 3 && GameClock_IsLeapYear(dateTime->year)) {
        day++;
    }
    return day;
}

bool GameClock_ToSeconds(const GameClockDateTime *dateTime, uint64_t *seconds)
{
    uint64_t days = 0;

    if (seconds == NULL || GameClock_DayOfYear(dateTime) == 0
        || dateTime->hour >= 24 || dateTime->minute >= 60
        || dateTime->second >= 60) {
        return false;
    }
    for (uint16_t year = GAME_CLOCK_FIRST_YEAR; year < dateTime->year; year++) {
        days += GameClock_IsLeapYear(year) ? 366 : 365;
    }
    days += (uint64_t)GameClock_DayOfYear(dateTime) - 1;
    *seconds = days * GAME_CLOCK_SECONDS_PER_DAY
        + dateTime->hour * 3600u + dateTime->minute * 60u + dateTime->second;
    return true;
}

bool GameClock_FromSeconds(uint64_t seconds, GameClockDateTime *dateTime)
{
    uint64_t days;
    uint32_t daySeconds;
    uint16_t year = GAME_CLOCK_FIRST_YEAR;
    uint8_t month = 1;

    if (dateTime == NULL
        || seconds >= (uint64_t)GAME_CLOCK_DAYS_PER_CYCLE * GAME_CLOCK_SECONDS_PER_DAY) {
        return false;
    }
    days = seconds / GAME_CLOCK_SECONDS_PER_DAY;
    daySeconds = seconds % GAME_CLOCK_SECONDS_PER_DAY;
    while (days >= (uint64_t)(GameClock_IsLeapYear(year) ? 366 : 365)) {
        days -= GameClock_IsLeapYear(year) ? 366 : 365;
        year++;
    }
    while (days >= GameClock_DaysInMonth(year, month)) {
        days -= GameClock_DaysInMonth(year, month);
        month++;
    }
    dateTime->year = year;
    dateTime->month = month;
    dateTime->day = (uint8_t)days + 1;
    dateTime->weekDay = (uint8_t)((seconds / GAME_CLOCK_SECONDS_PER_DAY + 6) % 7);
    dateTime->hour = daySeconds / 3600;
    dateTime->minute = (daySeconds % 3600) / 60;
    dateTime->second = daySeconds % 60;
    return true;
}
