#ifndef POKEPLATINUM_PLATFORM_ARCHIVE_H
#define POKEPLATINUM_PLATFORM_ARCHIVE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct PlatformNarc {
    const unsigned char *data;
    size_t size;
    size_t allocationTableOffset;
    size_t fileDataOffset;
    uint16_t memberCount;
} PlatformNarc;

bool PlatformNarc_Open(PlatformNarc *archive, const void *data, size_t size);
uint16_t PlatformNarc_GetMemberCount(const PlatformNarc *archive);
bool PlatformNarc_GetMember(const PlatformNarc *archive, uint16_t index,
    const void **data, size_t *size);

#endif
