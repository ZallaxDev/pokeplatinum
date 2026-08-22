#include "platform/archive.h"

#include <string.h>

#define NARC_HEADER_SIZE 16
#define NARC_BLOCK_HEADER_SIZE 8
#define NARC_ALLOCATION_ENTRY_SIZE 8

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

bool PlatformNarc_Open(PlatformNarc *archive, const void *data, size_t size)
{
    const unsigned char *bytes = data;
    size_t allocationBlockOffset = NARC_HEADER_SIZE;
    size_t nameBlockOffset;
    size_t fileBlockOffset;
    size_t allocationBlockSize;
    size_t nameBlockSize;
    size_t fileBlockSize;
    uint16_t memberCount;

    if (archive == NULL || data == NULL || !HasRange(size, 0, NARC_HEADER_SIZE)
        || memcmp(bytes, "NARC", 4) != 0 || ReadU16(bytes + 4) != 0xFFFE
        || ReadU16(bytes + 6) != 0x0100 || ReadU32(bytes + 8) != size
        || ReadU16(bytes + 12) != NARC_HEADER_SIZE || ReadU16(bytes + 14) != 3
        || !HasRange(size, allocationBlockOffset, NARC_BLOCK_HEADER_SIZE)
        || memcmp(bytes + allocationBlockOffset, "BTAF", 4) != 0) {
        return false;
    }

    allocationBlockSize = ReadU32(bytes + allocationBlockOffset + 4);
    memberCount = ReadU16(bytes + allocationBlockOffset + 8);
    if (allocationBlockSize < 12
        || !HasRange(size, allocationBlockOffset, allocationBlockSize)
        || (size_t)memberCount > (allocationBlockSize - 12) / NARC_ALLOCATION_ENTRY_SIZE) {
        return false;
    }

    nameBlockOffset = allocationBlockOffset + allocationBlockSize;
    if (!HasRange(size, nameBlockOffset, NARC_BLOCK_HEADER_SIZE)
        || memcmp(bytes + nameBlockOffset, "BTNF", 4) != 0) {
        return false;
    }
    nameBlockSize = ReadU32(bytes + nameBlockOffset + 4);
    if (nameBlockSize < NARC_BLOCK_HEADER_SIZE
        || !HasRange(size, nameBlockOffset, nameBlockSize)) {
        return false;
    }

    fileBlockOffset = nameBlockOffset + nameBlockSize;
    if (!HasRange(size, fileBlockOffset, NARC_BLOCK_HEADER_SIZE)
        || memcmp(bytes + fileBlockOffset, "GMIF", 4) != 0) {
        return false;
    }
    fileBlockSize = ReadU32(bytes + fileBlockOffset + 4);
    if (fileBlockSize < NARC_BLOCK_HEADER_SIZE
        || !HasRange(size, fileBlockOffset, fileBlockSize)
        || fileBlockOffset + fileBlockSize != size) {
        return false;
    }

    for (uint16_t index = 0; index < memberCount; index++) {
        size_t entryOffset = allocationBlockOffset + 12
            + (size_t)index * NARC_ALLOCATION_ENTRY_SIZE;
        uint32_t start = ReadU32(bytes + entryOffset);
        uint32_t end = ReadU32(bytes + entryOffset + 4);

        if (start > end || end > fileBlockSize - NARC_BLOCK_HEADER_SIZE) {
            return false;
        }
    }

    archive->data = bytes;
    archive->size = size;
    archive->allocationTableOffset = allocationBlockOffset + 12;
    archive->fileDataOffset = fileBlockOffset + NARC_BLOCK_HEADER_SIZE;
    archive->memberCount = memberCount;
    return true;
}

uint16_t PlatformNarc_GetMemberCount(const PlatformNarc *archive)
{
    return archive == NULL ? 0 : archive->memberCount;
}

bool PlatformNarc_GetMember(const PlatformNarc *archive, uint16_t index,
    const void **data, size_t *size)
{
    size_t entryOffset;
    uint32_t start;
    uint32_t end;

    if (archive == NULL || data == NULL || size == NULL || index >= archive->memberCount) {
        return false;
    }
    entryOffset = archive->allocationTableOffset
        + (size_t)index * NARC_ALLOCATION_ENTRY_SIZE;
    start = ReadU32(archive->data + entryOffset);
    end = ReadU32(archive->data + entryOffset + 4);
    if (start > end || !HasRange(archive->size, archive->fileDataOffset + start, end - start)) {
        return false;
    }

    *data = archive->data + archive->fileDataOffset + start;
    *size = end - start;
    return true;
}
