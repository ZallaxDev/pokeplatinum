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
typedef int OSArenaId;
typedef int OSHeapHandle;
typedef int OSIntrMode;
typedef int OSProcMode;

enum {
    OS_ARENA_MAIN = 0,
    OS_ARENA_MAINEX = 1,
    OS_PROCMODE_USER = 0,
    OS_PROCMODE_IRQ = 1,
};

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

void ErrorHandling_AssertFail(void);
void *OS_AllocFromMainArenaLo(u32 size, u32 alignment);
void *OS_AllocFromMainExArenaHi(u32 size, u32 alignment);
void *OS_AllocFromHeap(OSArenaId arena, OSHeapHandle heap, u32 size);
void OS_FreeToHeap(OSArenaId arena, OSHeapHandle heap, void *ptr);
OSProcMode OS_GetProcMode(void);
OSIntrMode OS_DisableInterrupts(void);
void OS_RestoreInterrupts(OSIntrMode mode);
int OS_Printf(const char *format, ...);
void MI_CpuFill32(void *dest, u32 value, u32 size);

#ifndef GF_ASSERT
#define GF_ASSERT(condition) ((condition) ? (void)0 : ErrorHandling_AssertFail())
#endif

#ifndef SDK_ASSERT
#define SDK_ASSERT(condition) GF_ASSERT(condition)
#endif

#ifndef SDK_NULL_ASSERT
#define SDK_NULL_ASSERT(value) GF_ASSERT((value) != NULL)
#endif

#include "nitro/fs.h"

#endif // POKEPLATINUM_3DS_COMPAT_H
