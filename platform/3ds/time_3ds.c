#include "platform/time.h"

#include <3ds.h>

#define NANOSECONDS_PER_SECOND 1000000000ULL

uint64_t PlatformTime_GetMonotonicNs(void)
{
    uint64_t systemTicks = svcGetSystemTick();
    uint64_t seconds = systemTicks / SYSCLOCK_ARM11;
    uint64_t remainder = systemTicks % SYSCLOCK_ARM11;

    return seconds * NANOSECONDS_PER_SECOND
        + remainder * NANOSECONDS_PER_SECOND / SYSCLOCK_ARM11;
}

void PlatformTickScheduler_Init(PlatformTickScheduler *scheduler)
{
    scheduler->lastTimeNs = PlatformTime_GetMonotonicNs();
    scheduler->elapsedNs = 0;
    scheduler->scaledRemainder = 0;
    scheduler->tickCount = 0;
}

uint32_t PlatformTickScheduler_Update(PlatformTickScheduler *scheduler)
{
    const uint64_t threshold = NANOSECONDS_PER_SECOND * PLATFORM_GAME_TICK_RATE_DENOMINATOR;
    uint64_t now = PlatformTime_GetMonotonicNs();
    uint64_t elapsed = now - scheduler->lastTimeNs;
    uint64_t scaledElapsed = elapsed * PLATFORM_GAME_TICK_RATE_NUMERATOR;
    uint64_t ticksDue;

    scheduler->lastTimeNs = now;
    scheduler->elapsedNs += elapsed;
    scheduler->scaledRemainder += scaledElapsed;
    ticksDue = scheduler->scaledRemainder / threshold;
    scheduler->scaledRemainder %= threshold;
    scheduler->tickCount += ticksDue;

    return (uint32_t)ticksDue;
}
