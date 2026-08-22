#include "platform/nitro2d.h"

#include <string.h>

#define NITRO_HEADER_SIZE 16
#define NCGR_BLOCK_HEADER_SIZE 32
#define NCLR_BLOCK_HEADER_SIZE 24

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
            const unsigned char *colorData = palette->colors
                + (paletteOffset + index) * 2;
            uint16_t color = ReadU16(colorData);
            uint32_t red = (color & 0x1F) * 255 / 31;
            uint32_t green = ((color >> 5) & 0x1F) * 255 / 31;
            uint32_t blue = ((color >> 10) & 0x1F) * 255 / 31;

            pixels[(size_t)y * width + x] = red | green << 8 | blue << 16
                | (index == 0 ? 0u : 255u) << 24;
        }
    }
    return true;
}
