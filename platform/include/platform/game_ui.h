#ifndef POKEPLATINUM_PLATFORM_GAME_UI_H
#define POKEPLATINUM_PLATFORM_GAME_UI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/nitro2d.h"

typedef struct GameFont {
    const unsigned char *glyphs;
    const unsigned char *widths;
    size_t glyphDataSize;
    uint32_t glyphCount;
    uint8_t maxWidth;
    uint8_t maxHeight;
    uint8_t tileWidth;
    uint8_t tileHeight;
} GameFont;

typedef struct GameDialoguePrinter {
    const uint16_t *text;
    const GameFont *font;
    const PlatformNclr *palette;
    uint32_t *pixels;
    size_t textLength;
    size_t cursor;
    unsigned int x;
    unsigned int y;
    unsigned int color;
    unsigned int delay;
    unsigned int speed;
    bool complete;
} GameDialoguePrinter;

bool GameMessageBank_Decode(const void *data, size_t size, unsigned int entryId,
    uint16_t *text, size_t capacity, size_t *textLength);
bool GameFont_Open(GameFont *font, const void *data, size_t size);
unsigned int GameFont_MeasureLongestLine(const GameFont *font,
    const uint16_t *text, size_t textLength);
bool GameDialogue_DrawWindow(uint32_t *pixels, size_t pixelCount,
    const PlatformNcgr *frame, const PlatformNclr *framePalette,
    const PlatformNclr *fontPalette);
bool GameDialoguePrinter_Init(GameDialoguePrinter *printer,
    const uint16_t *text, size_t textLength, const GameFont *font,
    const PlatformNclr *palette, uint32_t *pixels);
bool GameDialoguePrinter_Tick(GameDialoguePrinter *printer, bool *changed);

#endif
