#ifndef POKEPLATINUM_PLATFORM_TIME_H
#define POKEPLATINUM_PLATFORM_TIME_H

#include <stdint.h>

#define PLATFORM_GAME_TICK_RATE_NUMERATOR 598261ULL
#define PLATFORM_GAME_TICK_RATE_DENOMINATOR 10000ULL

typedef struct PlatformTickScheduler {
    uint64_t lastTimeNs;
    uint64_t elapsedNs;
    uint64_t scaledRemainder;
    uint64_t tickCount;
} PlatformTickScheduler;

uint64_t PlatformTime_GetMonotonicNs(void);
void PlatformTickScheduler_Init(PlatformTickScheduler *scheduler);
uint32_t PlatformTickScheduler_Update(PlatformTickScheduler *scheduler);

#endif
