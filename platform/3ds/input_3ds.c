#include "platform/input.h"

#include <3ds.h>
#include <string.h>

static uint32_t ConvertKeys(u32 keys)
{
    uint32_t result = 0;

    if (keys & KEY_A) result |= GAME_KEY_A;
    if (keys & KEY_B) result |= GAME_KEY_B;
    if (keys & KEY_X) result |= GAME_KEY_X;
    if (keys & KEY_Y) result |= GAME_KEY_Y;
    if (keys & KEY_L) result |= GAME_KEY_L;
    if (keys & KEY_R) result |= GAME_KEY_R;
    if (keys & KEY_START) result |= GAME_KEY_START;
    if (keys & KEY_SELECT) result |= GAME_KEY_SELECT;
    if (keys & KEY_DUP) result |= GAME_KEY_UP;
    if (keys & KEY_DDOWN) result |= GAME_KEY_DOWN;
    if (keys & KEY_DLEFT) result |= GAME_KEY_LEFT;
    if (keys & KEY_DRIGHT) result |= GAME_KEY_RIGHT;

    return result;
}

void PlatformInput_Update(PlatformInputState *state)
{
    touchPosition touch;
    bool wasTouchHeld = state->touchHeld;

    hidScanInput();
    memset(state, 0, sizeof(*state));
    state->held = ConvertKeys(hidKeysHeld());
    state->pressed = ConvertKeys(hidKeysDown());
    state->released = ConvertKeys(hidKeysUp());
    state->touchHeld = (hidKeysHeld() & KEY_TOUCH) != 0;
    state->touchPressed = state->touchHeld && !wasTouchHeld;

    if (state->touchHeld) {
        hidTouchRead(&touch);
        state->touchX = (uint16_t)((touch.px * 256u) / 320u);
        state->touchY = (uint16_t)((touch.py * 192u) / 240u);
    }
}
