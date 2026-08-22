#ifndef POKEPLATINUM_3DS_COMPAT_H
#define POKEPLATINUM_3DS_COMPAT_H

#include <stddef.h>
#include <stdint.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;
typedef int BOOL;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

void ErrorHandling_AssertFail(void);

#ifndef GF_ASSERT
#define GF_ASSERT(condition) ((condition) ? (void)0 : ErrorHandling_AssertFail())
#endif

#include "nitro/fs.h"

#endif // POKEPLATINUM_3DS_COMPAT_H
