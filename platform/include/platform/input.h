#ifndef POKEPLATINUM_PLATFORM_INPUT_H
#define POKEPLATINUM_PLATFORM_INPUT_H

#include <stdbool.h>
#include <stdint.h>

enum GameKey {
    GAME_KEY_A = 1 << 0,
    GAME_KEY_B = 1 << 1,
    GAME_KEY_X = 1 << 2,
    GAME_KEY_Y = 1 << 3,
    GAME_KEY_L = 1 << 4,
    GAME_KEY_R = 1 << 5,
    GAME_KEY_START = 1 << 6,
    GAME_KEY_SELECT = 1 << 7,
    GAME_KEY_UP = 1 << 8,
    GAME_KEY_DOWN = 1 << 9,
    GAME_KEY_LEFT = 1 << 10,
    GAME_KEY_RIGHT = 1 << 11
};

typedef struct PlatformInputState {
    uint32_t held;
    uint32_t pressed;
    uint32_t released;
    uint16_t touchX;
    uint16_t touchY;
    bool touchHeld;
    bool touchPressed;
} PlatformInputState;

void PlatformInput_Update(PlatformInputState *state);

#endif
