#include "platform/nitro2d.h"

#include <string.h>

#define NITRO_HEADER_SIZE 16
#define NCGR_BLOCK_HEADER_SIZE 32
#define NCLR_BLOCK_HEADER_SIZE 24
#define NSCR_BLOCK_HEADER_SIZE 20

static uint16_t ReadU16(const unsigned char *data)
{
    return (uint16_t)data[0] | (uint16_t)data[1] << 8;
}

static uint32_t ReadU32(const unsigned char *data)
{
    return (uint32_t)data[0] | (uint32_t)data[1] << 8
        | (uint32_t)data[2] << 16 | (uint32_t)data[3] << 24;
}

static bool HasRange(size_t totalSize, size_t offset, size_t length)
{
    return offset <= totalSize && length <= totalSize - offset;
}

static bool HasNitroHeader(const unsigned char *data, size_t size, const char *signature)
{
    return data != NULL && HasRange(size, 0, NITRO_HEADER_SIZE)
        && memcmp(data, signature, 4) == 0
        && ReadU16(data + 4) == 0xFEFF
        && ReadU32(data + 8) == size
        && ReadU16(data + 12) == NITRO_HEADER_SIZE
        && ReadU16(data + 14) >= 1;
}

bool PlatformNcgr_Open(PlatformNcgr *image, const void *data, size_t size)
{
    const unsigned char *bytes = data;
    const unsigned char *block;
    size_t blockSize;
    size_t dataSize;

    if (image == NULL || !HasNitroHeader(bytes, size, "RGCN")
        || !HasRange(size, NITRO_HEADER_SIZE, NCGR_BLOCK_HEADER_SIZE)) {
        return false;
    }
    block = bytes + NITRO_HEADER_SIZE;
    blockSize = ReadU32(block + 4);
    dataSize = ReadU32(block + 24);
    if (memcmp(block, "RAHC", 4) != 0 || blockSize < NCGR_BLOCK_HEADER_SIZE
        || !HasRange(size, NITRO_HEADER_SIZE, blockSize)
        || block[12] != 3 || block[20] != 0
        || dataSize > blockSize - NCGR_BLOCK_HEADER_SIZE) {
        return false;
    }

    image->tiles = block + NCGR_BLOCK_HEADER_SIZE;
    image->tileDataSize = dataSize;
    image->bitsPerPixel = 4;
    return true;
}

bool PlatformNclr_Open(PlatformNclr *palette, const void *data, size_t size)
{
    const unsigned char *bytes = data;
    const unsigned char *block;
    size_t blockSize;
    size_t dataSize;
    size_t declaredSize;

    if (palette == NULL || !HasNitroHeader(bytes, size, "RLCN")
        || !HasRange(size, NITRO_HEADER_SIZE, NCLR_BLOCK_HEADER_SIZE)) {
        return false;
    }
    block = bytes + NITRO_HEADER_SIZE;
    blockSize = ReadU32(block + 4);
    if (memcmp(block, "TTLP", 4) != 0 || blockSize < NCLR_BLOCK_HEADER_SIZE
        || !HasRange(size, NITRO_HEADER_SIZE, blockSize) || block[8] != 3) {
        return false;
    }
    dataSize = blockSize - NCLR_BLOCK_HEADER_SIZE;
    declaredSize = ReadU32(block + 16);
    if (declaredSize != dataSize
        && !(declaredSize <= 0x200 && 0x200 - declaredSize == dataSize)) {
        return false;
    }

    palette->colors = block + NCLR_BLOCK_HEADER_SIZE;
    palette->colorCount = dataSize / 2;
    palette->bitsPerPixel = 4;
    return (dataSize & 1) == 0;
}

bool PlatformNscr_Open(PlatformNscr *screen, const void *data, size_t size)
{
    const unsigned char *bytes = data;
    const unsigned char *block;
    size_t blockSize;
    size_t dataSize;
    unsigned int width;
    unsigned int height;

    if (screen == NULL || !HasNitroHeader(bytes, size, "RCSN")
        || !HasRange(size, NITRO_HEADER_SIZE, NSCR_BLOCK_HEADER_SIZE)) {
        return false;
    }
    block = bytes + NITRO_HEADER_SIZE;
    blockSize = ReadU32(block + 4);
    width = ReadU16(block + 8);
    height = ReadU16(block + 10);
    dataSize = ReadU32(block + 16);
    if (memcmp(block, "NRCS", 4) != 0 || blockSize < NSCR_BLOCK_HEADER_SIZE
        || !HasRange(size, NITRO_HEADER_SIZE, blockSize)
        || width == 0 || height == 0 || (width & 7) != 0 || (height & 7) != 0
        || ReadU16(block + 12) != 0 || ReadU16(block + 14) != 0
        || dataSize != (size_t)(width / 8) * (height / 8) * sizeof(uint16_t)
        || dataSize > blockSize - NSCR_BLOCK_HEADER_SIZE) {
        return false;
    }
    screen->entries = block + NSCR_BLOCK_HEADER_SIZE;
    screen->entryDataSize = dataSize;
    screen->width = width;
    screen->height = height;
    screen->bitsPerPixel = 4;
    return true;
}

static uint32_t DecodeColor(const PlatformNclr *palette, size_t colorIndex,
    bool transparent)
{
    uint16_t color = ReadU16(palette->colors + colorIndex * 2);
    uint32_t red = (color & 0x1F) * 255 / 31;
    uint32_t green = ((color >> 5) & 0x1F) * 255 / 31;
    uint32_t blue = ((color >> 10) & 0x1F) * 255 / 31;

    return red | green << 8 | blue << 16 | (transparent ? 0u : 255u) << 24;
}

bool PlatformNclr_GetColor(const PlatformNclr *palette, size_t colorIndex,
    bool transparent, uint32_t *color)
{
    if (palette == NULL || color == NULL || colorIndex >= palette->colorCount) {
        return false;
    }
    *color = DecodeColor(palette, colorIndex, transparent);
    return true;
}

bool PlatformNitro2D_DecodeTiles4Bpp(const PlatformNcgr *image,
    const PlatformNclr *palette, unsigned int paletteBank, unsigned int firstTile,
    unsigned int width, unsigned int height, uint32_t *pixels, size_t pixelCount)
{
    size_t tileCount;
    size_t availableTiles;
    size_t paletteOffset;

    if (image == NULL || palette == NULL || pixels == NULL
        || image->bitsPerPixel != 4 || palette->bitsPerPixel != 4
        || width == 0 || height == 0 || (width & 7) != 0 || (height & 7) != 0
        || pixelCount < (size_t)width * height) {
        return false;
    }
    tileCount = (size_t)(width / 8) * (height / 8);
    availableTiles = image->tileDataSize / 32;
    if (palette->colorCount < 16
        || (size_t)paletteBank > (palette->colorCount - 16) / 16) {
        return false;
    }
    paletteOffset = (size_t)paletteBank * 16;
    if ((size_t)firstTile > availableTiles
        || tileCount > availableTiles - firstTile
        || paletteOffset > palette->colorCount - 16) {
        return false;
    }

    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            unsigned int tile = firstTile + (y / 8) * (width / 8) + x / 8;
            size_t byteOffset = (size_t)tile * 32 + (y & 7) * 4 + (x & 7) / 2;
            unsigned int index = (x & 1)
                ? image->tiles[byteOffset] >> 4
                : image->tiles[byteOffset] & 0xF;
            pixels[(size_t)y * width + x] = DecodeColor(palette,
                paletteOffset + index, index == 0);
        }
    }
    return true;
}

bool PlatformNitro2D_DecodeTextBg4Bpp(const PlatformNcgr *image,
    const PlatformNclr *palette, const PlatformNscr *screen,
    unsigned int baseTile, unsigned int width, unsigned int height,
    bool transparentZero, uint32_t *pixels, size_t pixelCount)
{
    size_t availableTiles;
    size_t mapWidth;

    if (image == NULL || palette == NULL || screen == NULL || pixels == NULL
        || image->bitsPerPixel != 4 || palette->bitsPerPixel != 4
        || screen->bitsPerPixel != 4 || width == 0 || height == 0
        || (width & 7) != 0 || (height & 7) != 0
        || width > screen->width || height > screen->height
        || screen->entryDataSize < (size_t)(screen->width / 8)
                * (screen->height / 8) * sizeof(uint16_t)
        || pixelCount < (size_t)width * height || palette->colorCount < 16) {
        return false;
    }
    availableTiles = image->tileDataSize / 32;
    if (baseTile > availableTiles) {
        return false;
    }
    mapWidth = screen->width / 8;
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            size_t entryOffset = ((size_t)(y / 8) * mapWidth + x / 8) * 2;
            uint16_t entry = ReadU16(screen->entries + entryOffset);
            size_t tileOffset = entry & 0x3FF;
            size_t tile;
            size_t paletteOffset = (size_t)(entry >> 12) * 16;
            unsigned int tileX = x & 7;
            unsigned int tileY = y & 7;
            size_t byteOffset;
            unsigned int index;

            if (entry & 0x400) {
                tileX = 7 - tileX;
            }
            if (entry & 0x800) {
                tileY = 7 - tileY;
            }
            if (tileOffset >= availableTiles - baseTile
                || paletteOffset > palette->colorCount - 16) {
                return false;
            }
            tile = baseTile + tileOffset;
            byteOffset = tile * 32 + tileY * 4 + tileX / 2;
            index = (tileX & 1)
                ? image->tiles[byteOffset] >> 4
                : image->tiles[byteOffset] & 0xF;
            pixels[(size_t)y * width + x] = DecodeColor(palette,
                paletteOffset + index, transparentZero && index == 0);
        }
    }
    return true;
}
