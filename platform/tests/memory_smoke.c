#include "platform/memory.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    void *low;
    void *high;
    void *reused;
    void *upperChild;

    PlatformHeap_Init();
    if (!PlatformHeap_Create(PLATFORM_HEAP_SYSTEM, 4096)
        || !PlatformHeap_CreateChild(PLATFORM_HEAP_SYSTEM, 10, 1024, false)
        || !PlatformHeap_CreateChild(PLATFORM_HEAP_SYSTEM, 11, 1024, true)
        || PlatformHeap_GetAllocatedSize(PLATFORM_HEAP_SYSTEM) != 2048
        || PlatformHeap_Destroy(PLATFORM_HEAP_SYSTEM)) {
        fprintf(stderr, "GAME HEAP SMOKE FAILED: hierarchy\n");
        return 1;
    }
    low = PlatformHeap_Alloc(10, 128, 32);
    high = PlatformHeap_AllocAtEnd(10, 128, 64);
    upperChild = PlatformHeap_Alloc(11, 128, 16);
    if (low == NULL || high == NULL || (uintptr_t)low % 32 != 0
        || upperChild == NULL || (uintptr_t)high % 64 != 0
        || (uintptr_t)low >= (uintptr_t)high
        || (uintptr_t)high >= (uintptr_t)upperChild
        || !PlatformHeap_Free(10, low)) {
        fprintf(stderr, "GAME HEAP SMOKE FAILED: direction/alignment\n");
        return 1;
    }
    reused = PlatformHeap_Alloc(10, 128, 32);
    if (reused != low || !PlatformHeap_Free(10, reused)
        || !PlatformHeap_Free(10, high) || !PlatformHeap_Free(11, upperChild)
        || !PlatformHeap_Destroy(10)
        || !PlatformHeap_Destroy(11)
        || PlatformHeap_GetAllocatedSize(PLATFORM_HEAP_SYSTEM) != 0
        || !PlatformHeap_Destroy(PLATFORM_HEAP_SYSTEM)) {
        fprintf(stderr, "GAME HEAP SMOKE FAILED: reuse/destroy\n");
        return 1;
    }
    PlatformHeap_Shutdown();
    printf("GAME HEAP SMOKE OK: hierarchy, low/high, alignment, reuse\n");
    return 0;
}
