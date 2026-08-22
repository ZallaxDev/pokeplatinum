#include "platform/nitro2d.h"

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

static void InitHeader(unsigned char *data, const char *signature,
    size_t size, const char *blockSignature, size_t blockSize)
{
    memcpy(data, signature, 4);
    WriteU16(data + 4, 0xFEFF);
    WriteU16(data + 6, 0x100);
    WriteU32(data + 8, size);
    WriteU16(data + 12, 16);
    WriteU16(data + 14, 1);
    memcpy(data + 16, blockSignature, 4);
    WriteU32(data + 20, blockSize);
}

int main(void)
{
    unsigned char ncgr[80] = { 0 };
    unsigned char nclr[104] = { 0 };
    unsigned char nscr[40] = { 0 };
    uint32_t pixels[128];
    PlatformNcgr image;
    PlatformNclr palette;
    PlatformNscr screen;
    bool imageOk;
    bool paletteOk;
    bool screenOk;
    bool decodeOk;

    InitHeader(ncgr, "RGCN", sizeof(ncgr), "RAHC", 64);
    ncgr[28] = 3;
    WriteU32(ncgr + 40, 32);
    for (int row = 0; row < 8; row++) {
        ncgr[48 + row * 4] = 0x21;
        ncgr[49 + row * 4] = 0x43;
        ncgr[50 + row * 4] = 0x65;
        ncgr[51 + row * 4] = 0x87;
    }
    ncgr[48 + 7 * 4] = 0x29;
    InitHeader(nclr, "RLCN", sizeof(nclr), "TTLP", 88);
    nclr[24] = 3;
    WriteU32(nclr + 32, 64);
    WriteU16(nclr + 40 + 8 * 2, 0x001F);
    WriteU16(nclr + 40 + 17 * 2, 0x03E0);
    WriteU16(nclr + 40 + 25 * 2, 0x7C00);
    InitHeader(nscr, "RCSN", sizeof(nscr), "NRCS", 24);
    WriteU16(nscr + 24, 16);
    WriteU16(nscr + 26, 8);
    WriteU32(nscr + 32, 4);
    WriteU16(nscr + 36, 0x0400);
    WriteU16(nscr + 38, 0x1800);

    imageOk = PlatformNcgr_Open(&image, ncgr, sizeof(ncgr));
    paletteOk = PlatformNclr_Open(&palette, nclr, sizeof(nclr));
    screenOk = PlatformNscr_Open(&screen, nscr, sizeof(nscr));
    decodeOk = imageOk && paletteOk && screenOk
        && PlatformNitro2D_DecodeTextBg4Bpp(&image, &palette, &screen,
            0, 16, 8, false, pixels, 128);
    if (!imageOk || !paletteOk || !screenOk || !decodeOk
        || pixels[0] != 0xFF0000FFu || pixels[7] == pixels[0]
        || pixels[8] != 0xFFFF0000u) {
        fprintf(stderr, "NITRO2D BG SMOKE FAILED: open=%d/%d/%d decode=%d pixels=%08x/%08x\n",
            imageOk, paletteOk, screenOk, decodeOk, pixels[0], pixels[7]);
        return 1;
    }
    printf("NITRO2D BG SMOKE OK: NSCR palette banks and H/V flips\n");
    return 0;
}
