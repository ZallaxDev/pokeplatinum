#include "platform/game_fade.h"

#include <stddef.h>

bool GameFade_Start(GameFade *fade, bool fadeIn, unsigned int steps,
    unsigned int framesPerStep)
{
    if (fade == NULL || steps == 0 || framesPerStep == 0) return false;
    fade->step = 0;
    fade->steps = steps;
    fade->frame = 0;
    fade->framesPerStep = framesPerStep;
    fade->fadeIn = fadeIn;
    fade->active = true;
    return true;
}

void GameFade_Tick(GameFade *fade)
{
    if (fade == NULL || !fade->active) return;
    fade->frame++;
    if (fade->frame >= fade->framesPerStep) {
        fade->frame = 0;
        fade->step++;
        if (fade->step >= fade->steps) fade->active = false;
    }
}

bool GameFade_IsDone(const GameFade *fade)
{
    return fade == NULL || !fade->active;
}

uint8_t GameFade_GetBlackAlpha(const GameFade *fade)
{
    unsigned int progress;

    if (fade == NULL || fade->steps == 0) return 0;
    progress = fade->active ? fade->step : fade->steps;
    return fade->fadeIn
        ? (uint8_t)(255 - progress * 255 / fade->steps)
        : (uint8_t)(progress * 255 / fade->steps);
}
