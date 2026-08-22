#ifndef POKEPLATINUM_3DS_NITRO_RTC_H
#define POKEPLATINUM_3DS_NITRO_RTC_H

#include "pokeplatinum_compat.h"

typedef enum RTCWeek {
    RTC_WEEK_SUNDAY = 0,
    RTC_WEEK_MONDAY,
    RTC_WEEK_TUESDAY,
    RTC_WEEK_WEDNESDAY,
    RTC_WEEK_THURSDAY,
    RTC_WEEK_FRIDAY,
    RTC_WEEK_SATURDAY,
    RTC_WEEK_MAX,
} RTCWeek;

typedef enum RTCResult {
    RTC_RESULT_SUCCESS = 0,
    RTC_RESULT_BUSY,
    RTC_RESULT_ILLEGAL_PARAMETER,
    RTC_RESULT_SEND_ERROR,
    RTC_RESULT_INVALID_COMMAND,
    RTC_RESULT_ILLEGAL_STATUS,
    RTC_RESULT_FATAL_ERROR,
    RTC_RESULT_MAX,
} RTCResult;

typedef struct RTCDate {
    u32 year;
    u32 month;
    u32 day;
    RTCWeek week;
} RTCDate;

typedef struct RTCTime {
    u32 hour;
    u32 minute;
    u32 second;
} RTCTime;

typedef void (*RTCCallback)(RTCResult result, void *arg);

void RTC_Init(void);
RTCResult RTC_GetDateTimeAsync(RTCDate *date, RTCTime *time, RTCCallback callback, void *arg);
s32 RTC_ConvertDateToDay(const RTCDate *date);
s64 RTC_ConvertDateTimeToSecond(const RTCDate *date, const RTCTime *time);
void RTC_ConvertDayToDate(RTCDate *date, s32 day);
void RTC_ConvertSecondToDateTime(RTCDate *date, RTCTime *time, s64 seconds);
RTCWeek RTC_GetDayOfWeek(RTCDate *date);

#endif // POKEPLATINUM_3DS_NITRO_RTC_H
