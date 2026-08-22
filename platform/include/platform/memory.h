#ifndef POKEPLATINUM_PLATFORM_MEMORY_H
#define POKEPLATINUM_PLATFORM_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum PlatformHeapID {
    PLATFORM_HEAP_SYSTEM = 0,
    PLATFORM_HEAP_DEBUG = 2,
    PLATFORM_HEAP_APPLICATION = 3,
};

void PlatformHeap_Init(void);
void PlatformHeap_Shutdown(void);
bool PlatformHeap_Create(uint32_t heapId, size_t capacity);
bool PlatformHeap_CreateChild(uint32_t parentHeapId, uint32_t childHeapId,
    size_t capacity, bool atEnd);
bool PlatformHeap_Destroy(uint32_t heapId);
void *PlatformHeap_Alloc(uint32_t heapId, size_t size, size_t alignment);
void *PlatformHeap_AllocAtEnd(uint32_t heapId, size_t size, size_t alignment);
bool PlatformHeap_Free(uint32_t heapId, void *allocation);
size_t PlatformHeap_GetCapacity(uint32_t heapId);
size_t PlatformHeap_GetAllocatedSize(uint32_t heapId);
size_t PlatformHeap_GetAllocationCount(uint32_t heapId);

#endif
