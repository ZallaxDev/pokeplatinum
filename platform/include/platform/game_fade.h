#ifndef POKEPLATINUM_PLATFORM_GAME_FADE_H
#define POKEPLATINUM_PLATFORM_GAME_FADE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct GameFade {
    unsigned int step;
    unsigned int steps;
    unsigned int frame;
    unsigned int framesPerStep;
    bool fadeIn;
    bool active;
} GameFade;

bool GameFade_Start(GameFade *fade, bool fadeIn, unsigned int steps,
    unsigned int framesPerStep);
void GameFade_Tick(GameFade *fade);
bool GameFade_IsDone(const GameFade *fade);
uint8_t GameFade_GetBlackAlpha(const GameFade *fade);

#endif
