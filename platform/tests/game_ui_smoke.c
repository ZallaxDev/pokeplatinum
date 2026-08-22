#include "platform/game_fade.h"
#include "platform/game_ui.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void WriteU16(unsigned char *data, uint16_t value)
{
    data[0] = value & 0xFF;
    data[1] = value >> 8;
}

static void WriteU32(unsigned char *data, uint32_t value)
{
    data[0] = value & 0xFF;
    data[1] = value >> 8;
    data[2] = value >> 16;
    data[3] = value >> 24;
}

int main(void)
{
    unsigned char bank[16] = { 0 };
    unsigned char fontData[67] = { 0 };
    unsigned char paletteData[64] = { 0 };
    unsigned char frameData[18 * 32];
    uint16_t decoded[2];
    uint16_t text[] = { 1, 0xFFFE, 0xFF00, 1, 2, 2, 0xE000, 3, 0xFFFF };
    uint32_t pixels[256 * 192] = { 0 };
    PlatformNclr palette = { paletteData, 32, 4 };
    PlatformNcgr frame = { frameData, sizeof(frameData), 4 };
    GameFont font;
    GameDialoguePrinter printer;
    GameFade fade;
    size_t decodedLength;
    uint32_t entrySeed = (123u * 765u) & 0xFFFF;
    uint16_t stringSeed = (uint16_t)596947u;
    int changedTicks[3] = { -1, -1, -1 };
    int changedCount = 0;

    entrySeed |= entrySeed << 16;
    WriteU16(bank, 1);
    WriteU16(bank + 2, 123);
    WriteU32(bank + 4, 12 ^ entrySeed);
    WriteU32(bank + 8, 2 ^ entrySeed);
    WriteU16(bank + 12, 1 ^ stringSeed);
    stringSeed = (uint16_t)(stringSeed + 18749);
    WriteU16(bank + 14, 0xFFFF ^ stringSeed);
    if (!GameMessageBank_Decode(bank, sizeof(bank), 0, decoded, 2,
            &decodedLength)
        || decodedLength != 2 || decoded[0] != 1 || decoded[1] != 0xFFFF) {
        fprintf(stderr, "GAME UI SMOKE FAILED: message decode\n");
        return 1;
    }

    WriteU32(fontData, 16);
    WriteU32(fontData + 4, 64);
    WriteU32(fontData + 8, 3);
    fontData[12] = 8;
    fontData[13] = 8;
    fontData[14] = 1;
    fontData[15] = 1;
    memset(fontData + 16, 0x55, 48);
    fontData[64] = 4;
    fontData[65] = 4;
    fontData[66] = 4;
    WriteU16(paletteData + 1 * 2, 0x001F);
    WriteU16(paletteData + 5 * 2, 0x03E0);
    WriteU16(paletteData + 15 * 2, 0x7FFF);
    memset(frameData, 0x11, sizeof(frameData));

    if (!GameFont_Open(&font, fontData, sizeof(fontData))
        || GameFont_MeasureLongestLine(&font, text,
            sizeof(text) / sizeof(text[0])) != 8
        || !GameDialogue_DrawWindow(pixels,
            sizeof(pixels) / sizeof(pixels[0]), &frame, &palette, &palette)
        || !GameDialoguePrinter_Init(&printer, text,
            sizeof(text) / sizeof(text[0]), &font, &palette, pixels)) {
        fprintf(stderr, "GAME UI SMOKE FAILED: setup\n");
        return 1;
    }
    for (int tick = 0; tick <= 13 && !printer.complete; tick++) {
        bool changed;
        if (!GameDialoguePrinter_Tick(&printer, &changed)) {
            fprintf(stderr, "GAME UI SMOKE FAILED: printer\n");
            return 1;
        }
        if (changed && changedCount < 3) changedTicks[changedCount++] = tick;
    }
    if (!printer.complete || changedCount != 3 || changedTicks[0] != 0
        || changedTicks[1] != 4 || changedTicks[2] != 8
        || pixels[152 * 256 + 16] != 0xFF0000FFu
        || pixels[152 * 256 + 20] != 0xFF00FF00u
        || pixels[160 * 256 + 16] != 0xFF00FF00u) {
        fprintf(stderr, "GAME UI SMOKE FAILED: timing/color/newline\n");
        return 1;
    }

    if (!GameFade_Start(&fade, true, 8, 1)
        || GameFade_GetBlackAlpha(&fade) != 255) {
        fprintf(stderr, "GAME UI SMOKE FAILED: fade start\n");
        return 1;
    }
    for (int tick = 0; tick < 8; tick++) GameFade_Tick(&fade);
    if (!GameFade_IsDone(&fade) || GameFade_GetBlackAlpha(&fade) != 0) {
        fprintf(stderr, "GAME UI SMOKE FAILED: fade duration\n");
        return 1;
    }
    printf("GAME UI SMOKE OK: decrypt, frame, color, wrap, timing, fade\n");
    return 0;
}
