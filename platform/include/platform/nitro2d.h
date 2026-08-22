#ifndef POKEPLATINUM_PLATFORM_NITRO2D_H
#define POKEPLATINUM_PLATFORM_NITRO2D_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct PlatformNcgr {
    const unsigned char *tiles;
    size_t tileDataSize;
    unsigned int bitsPerPixel;
} PlatformNcgr;

typedef struct PlatformNclr {
    const unsigned char *colors;
    size_t colorCount;
    unsigned int bitsPerPixel;
} PlatformNclr;

bool PlatformNcgr_Open(PlatformNcgr *image, const void *data, size_t size);
bool PlatformNclr_Open(PlatformNclr *palette, const void *data, size_t size);
bool PlatformNitro2D_DecodeTiles4Bpp(const PlatformNcgr *image,
    const PlatformNclr *palette, unsigned int paletteBank, unsigned int firstTile,
    unsigned int width, unsigned int height, uint32_t *pixels, size_t pixelCount);

#endif
