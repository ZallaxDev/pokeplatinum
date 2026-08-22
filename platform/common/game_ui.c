#include "platform/game_ui.h"

#include <string.h>

#define CHAR_CR 0xE000
#define CHAR_CONTROL_SET_COLOR 0xFF00
#define CHAR_FORMAT_ARG 0xFFFE
#define CHAR_EOS 0xFFFF
#define DIALOGUE_X 16
#define DIALOGUE_Y 152
#define DIALOGUE_WIDTH 216
#define DIALOGUE_HEIGHT 32
#define LOGICAL_WIDTH 256
#define LOGICAL_HEIGHT 192

static uint16_t ReadU16(const unsigned char *data)
{
    return (uint16_t)data[0] | (uint16_t)data[1] << 8;
}

static uint32_t ReadU32(const unsigned char *data)
{
    return (uint32_t)data[0] | (uint32_t)data[1] << 8
        | (uint32_t)data[2] << 16 | (uint32_t)data[3] << 24;
}

bool GameMessageBank_Decode(const void *data, size_t size, unsigned int entryId,
    uint16_t *text, size_t capacity, size_t *textLength)
{
    const unsigned char *bytes = data;
    uint16_t count;
    uint16_t bankSeed;
    uint32_t entrySeed;
    uint32_t offset;
    uint32_t length;
    uint16_t stringSeed;

    if (bytes == NULL || text == NULL || textLength == NULL || size < 4) {
        return false;
    }
    count = ReadU16(bytes);
    bankSeed = ReadU16(bytes + 2);
    if (entryId >= count || (size_t)entryId * 8 > size - 4
        || 8 > size - 4 - (size_t)entryId * 8) {
        return false;
    }
    entrySeed = (bankSeed * 765u * (entryId + 1)) & 0xFFFF;
    entrySeed |= entrySeed << 16;
    offset = ReadU32(bytes + 4 + entryId * 8) ^ entrySeed;
    length = ReadU32(bytes + 8 + entryId * 8) ^ entrySeed;
    if (length > capacity || offset > size
        || (size_t)length * sizeof(uint16_t) > size - offset) {
        return false;
    }
    stringSeed = (uint16_t)((entryId + 1) * 596947u);
    for (uint32_t i = 0; i < length; i++) {
        text[i] = ReadU16(bytes + offset + i * 2) ^ stringSeed;
        stringSeed = (uint16_t)(stringSeed + 18749);
    }
    *textLength = length;
    return true;
}

bool GameFont_Open(GameFont *font, const void *data, size_t size)
{
    const unsigned char *bytes = data;
    uint32_t glyphOffset;
    uint32_t widthOffset;
    uint32_t glyphCount;
    size_t glyphSize;

    if (font == NULL || bytes == NULL || size < 16) {
        return false;
    }
    glyphOffset = ReadU32(bytes);
    widthOffset = ReadU32(bytes + 4);
    glyphCount = ReadU32(bytes + 8);
    glyphSize = (size_t)bytes[14] * bytes[15] * 16;
    if (glyphOffset < 16 || glyphCount == 0 || glyphSize == 0
        || glyphOffset > size || glyphCount > (size - glyphOffset) / glyphSize
        || widthOffset < glyphOffset + glyphCount * glyphSize
        || widthOffset > size || glyphCount > size - widthOffset) {
        return false;
    }
    font->glyphs = bytes + glyphOffset;
    font->widths = bytes + widthOffset;
    font->glyphDataSize = glyphSize;
    font->glyphCount = glyphCount;
    font->maxWidth = bytes[12];
    font->maxHeight = bytes[13];
    font->tileWidth = bytes[14];
    font->tileHeight = bytes[15];
    return font->maxWidth != 0 && font->maxHeight != 0;
}

static bool IsColorCommand(const uint16_t *text, size_t length, size_t cursor)
{
    return cursor + 3 < length && text[cursor] == CHAR_FORMAT_ARG
        && text[cursor + 1] == CHAR_CONTROL_SET_COLOR && text[cursor + 2] == 1;
}

unsigned int GameFont_MeasureLongestLine(const GameFont *font,
    const uint16_t *text, size_t textLength)
{
    unsigned int width = 0;
    unsigned int longest = 0;

    if (font == NULL || text == NULL) {
        return 0;
    }
    for (size_t cursor = 0; cursor < textLength;) {
        uint16_t code = text[cursor];
        if (code == CHAR_EOS) {
            break;
        }
        if (code == CHAR_CR) {
            if (width > longest) longest = width;
            width = 0;
            cursor++;
        } else if (IsColorCommand(text, textLength, cursor)) {
            cursor += 4;
        } else {
            if (code == 0 || code > font->glyphCount) return 0;
            width += font->widths[code - 1];
            cursor++;
        }
    }
    return width > longest ? width : longest;
}

static bool DrawFrameTile(uint32_t *pixels, const PlatformNcgr *frame,
    const PlatformNclr *palette, unsigned int tile, unsigned int x, unsigned int y)
{
    if (tile >= frame->tileDataSize / 32 || x + 8 > LOGICAL_WIDTH
        || y + 8 > LOGICAL_HEIGHT) {
        return false;
    }
    for (unsigned int py = 0; py < 8; py++) {
        for (unsigned int px = 0; px < 8; px++) {
            size_t offset = tile * 32 + py * 4 + px / 2;
            unsigned int index = (px & 1)
                ? frame->tiles[offset] >> 4 : frame->tiles[offset] & 0xF;
            uint32_t color;
            if (index != 0) {
                if (!PlatformNclr_GetColor(palette, index, false, &color)) return false;
                pixels[(y + py) * LOGICAL_WIDTH + x + px] = color;
            }
        }
    }
    return true;
}

bool GameDialogue_DrawWindow(uint32_t *pixels, size_t pixelCount,
    const PlatformNcgr *frame, const PlatformNclr *framePalette,
    const PlatformNclr *fontPalette)
{
    static const uint8_t leftTiles[] = { 6, 7 };
    static const uint8_t rightTiles[] = { 9, 10, 11 };
    uint32_t fill;

    if (pixels == NULL || pixelCount < LOGICAL_WIDTH * LOGICAL_HEIGHT
        || frame == NULL || framePalette == NULL || fontPalette == NULL
        || frame->bitsPerPixel != 4
        || !PlatformNclr_GetColor(fontPalette, 15, false, &fill)) {
        return false;
    }
    for (unsigned int y = DIALOGUE_Y; y < DIALOGUE_Y + DIALOGUE_HEIGHT; y++) {
        for (unsigned int x = DIALOGUE_X; x < DIALOGUE_X + DIALOGUE_WIDTH; x++) {
            pixels[y * LOGICAL_WIDTH + x] = fill;
        }
    }
    for (unsigned int x = 0; x < 27; x++) {
        if (!DrawFrameTile(pixels, frame, framePalette, 2, DIALOGUE_X + x * 8, 144)
            || !DrawFrameTile(pixels, frame, framePalette, 14,
                DIALOGUE_X + x * 8, 184)) return false;
    }
    if (!DrawFrameTile(pixels, frame, framePalette, 0, 0, 144)
        || !DrawFrameTile(pixels, frame, framePalette, 1, 8, 144)
        || !DrawFrameTile(pixels, frame, framePalette, 3, 232, 144)
        || !DrawFrameTile(pixels, frame, framePalette, 4, 240, 144)
        || !DrawFrameTile(pixels, frame, framePalette, 5, 248, 144)
        || !DrawFrameTile(pixels, frame, framePalette, 12, 0, 184)
        || !DrawFrameTile(pixels, frame, framePalette, 13, 8, 184)
        || !DrawFrameTile(pixels, frame, framePalette, 15, 232, 184)
        || !DrawFrameTile(pixels, frame, framePalette, 16, 240, 184)
        || !DrawFrameTile(pixels, frame, framePalette, 17, 248, 184)) return false;
    for (unsigned int row = 0; row < 4; row++) {
        for (unsigned int i = 0; i < 2; i++) {
            if (!DrawFrameTile(pixels, frame, framePalette, leftTiles[i], i * 8,
                    DIALOGUE_Y + row * 8)) return false;
        }
        for (unsigned int i = 0; i < 3; i++) {
            if (!DrawFrameTile(pixels, frame, framePalette, rightTiles[i],
                    232 + i * 8, DIALOGUE_Y + row * 8)) return false;
        }
    }
    return true;
}

bool GameDialoguePrinter_Init(GameDialoguePrinter *printer,
    const uint16_t *text, size_t textLength, const GameFont *font,
    const PlatformNclr *palette, uint32_t *pixels)
{
    if (printer == NULL || text == NULL || textLength == 0 || font == NULL
        || palette == NULL || pixels == NULL) return false;
    memset(printer, 0, sizeof(*printer));
    printer->text = text;
    printer->textLength = textLength;
    printer->font = font;
    printer->palette = palette;
    printer->pixels = pixels;
    printer->speed = 4;
    return true;
}

static bool DrawGlyph(GameDialoguePrinter *printer, uint16_t code)
{
    const unsigned char *glyph;
    unsigned int glyphIndex;

    if (code == 0 || code > printer->font->glyphCount) return false;
    glyphIndex = code - 1;
    glyph = printer->font->glyphs + glyphIndex * printer->font->glyphDataSize;
    for (unsigned int y = 0; y < printer->font->maxHeight; y++) {
        for (unsigned int x = 0; x < printer->font->maxWidth; x++) {
            unsigned int tile = (y / 8) * printer->font->tileWidth + x / 8;
            size_t offset = tile * 16 + (y & 7) * 2 + (x & 7) / 4;
            unsigned int value = (glyph[offset] >> ((x & 3) * 2)) & 3;
            size_t colorIndex;
            uint32_t color;

            if (value == 0) continue;
            colorIndex = value == 1 ? printer->color * 2 + 1
                : value == 2 ? printer->color * 2 + 2 : 15;
            if (!PlatformNclr_GetColor(printer->palette, colorIndex, false, &color)) {
                return false;
            }
            if (printer->x + x < DIALOGUE_WIDTH
                && printer->y + y < DIALOGUE_HEIGHT) {
                printer->pixels[(DIALOGUE_Y + printer->y + y) * LOGICAL_WIDTH
                    + DIALOGUE_X + printer->x + x] = color;
            }
        }
    }
    printer->x += printer->font->widths[glyphIndex];
    return true;
}

bool GameDialoguePrinter_Tick(GameDialoguePrinter *printer, bool *changed)
{
    if (printer == NULL || changed == NULL || printer->complete) return false;
    *changed = false;
    if (printer->delay != 0) {
        printer->delay--;
        return true;
    }
    while (printer->cursor < printer->textLength) {
        uint16_t code = printer->text[printer->cursor];
        if (code == CHAR_EOS) {
            printer->complete = true;
            return true;
        }
        if (code == CHAR_CR) {
            printer->x = 0;
            printer->y += printer->font->maxHeight;
            printer->cursor++;
            continue;
        }
        if (IsColorCommand(printer->text, printer->textLength, printer->cursor)) {
            printer->color = printer->text[printer->cursor + 3];
            printer->cursor += 4;
            continue;
        }
        if (!DrawGlyph(printer, code)) return false;
        printer->cursor++;
        printer->delay = printer->speed - 1;
        *changed = true;
        return true;
    }
    return false;
}
