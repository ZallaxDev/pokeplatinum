#include "platform/memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PLATFORM_HEAP_COUNT 128
#define PLATFORM_ALLOCATION_MAGIC 0x50484D45u

typedef struct PlatformAllocation PlatformAllocation;

typedef struct PlatformHeap {
    PlatformAllocation *allocations;
    size_t capacity;
    size_t allocatedSize;
    size_t allocationCount;
    bool active;
} PlatformHeap;

struct PlatformAllocation {
    PlatformAllocation *previous;
    PlatformAllocation *next;
    void *rawAllocation;
    size_t size;
    uint32_t heapId;
    uint32_t magic;
};

static PlatformHeap sHeaps[PLATFORM_HEAP_COUNT];

static bool PlatformHeap_IsValidId(uint32_t heapId)
{
    return heapId < PLATFORM_HEAP_COUNT;
}

void PlatformHeap_Init(void)
{
    memset(sHeaps, 0, sizeof(sHeaps));
}

void PlatformHeap_Shutdown(void)
{
    for (uint32_t heapId = 0; heapId < PLATFORM_HEAP_COUNT; heapId++) {
        PlatformAllocation *allocation = sHeaps[heapId].allocations;

        while (allocation != NULL) {
            PlatformAllocation *next = allocation->next;
            free(allocation->rawAllocation);
            allocation = next;
        }
    }
    memset(sHeaps, 0, sizeof(sHeaps));
}

bool PlatformHeap_Create(uint32_t heapId, size_t capacity)
{
    if (!PlatformHeap_IsValidId(heapId) || capacity == 0 || sHeaps[heapId].active) {
        return false;
    }

    sHeaps[heapId].capacity = capacity;
    sHeaps[heapId].active = true;
    return true;
}

bool PlatformHeap_Destroy(uint32_t heapId)
{
    if (!PlatformHeap_IsValidId(heapId) || !sHeaps[heapId].active
        || sHeaps[heapId].allocationCount != 0) {
        return false;
    }

    memset(&sHeaps[heapId], 0, sizeof(sHeaps[heapId]));
    return true;
}

void *PlatformHeap_Alloc(uint32_t heapId, size_t size, size_t alignment)
{
    PlatformHeap *heap;
    PlatformAllocation *allocation;
    uintptr_t alignedAddress;
    void *rawAllocation;
    size_t overhead;

    if (!PlatformHeap_IsValidId(heapId) || !sHeaps[heapId].active || size == 0
        || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return NULL;
    }

    heap = &sHeaps[heapId];
    if (size > heap->capacity - heap->allocatedSize) {
        return NULL;
    }
    overhead = sizeof(PlatformAllocation) + alignment - 1;
    if (size > SIZE_MAX - overhead) {
        return NULL;
    }

    rawAllocation = malloc(size + overhead);
    if (rawAllocation == NULL) {
        return NULL;
    }
    alignedAddress = ((uintptr_t)rawAllocation + sizeof(PlatformAllocation) + alignment - 1)
        & ~(uintptr_t)(alignment - 1);
    allocation = (PlatformAllocation *)(alignedAddress - sizeof(PlatformAllocation));
    allocation->previous = NULL;
    allocation->next = heap->allocations;
    allocation->rawAllocation = rawAllocation;
    allocation->size = size;
    allocation->heapId = heapId;
    allocation->magic = PLATFORM_ALLOCATION_MAGIC;
    if (heap->allocations != NULL) {
        heap->allocations->previous = allocation;
    }
    heap->allocations = allocation;
    heap->allocatedSize += size;
    heap->allocationCount++;

    return (void *)alignedAddress;
}

bool PlatformHeap_Free(uint32_t heapId, void *allocationAddress)
{
    PlatformHeap *heap;
    PlatformAllocation *allocation;

    if (!PlatformHeap_IsValidId(heapId) || allocationAddress == NULL) {
        return false;
    }
    heap = &sHeaps[heapId];
    allocation = (PlatformAllocation *)((uintptr_t)allocationAddress - sizeof(PlatformAllocation));
    if (!heap->active || allocation->magic != PLATFORM_ALLOCATION_MAGIC
        || allocation->heapId != heapId) {
        return false;
    }

    if (allocation->previous != NULL) {
        allocation->previous->next = allocation->next;
    } else {
        heap->allocations = allocation->next;
    }
    if (allocation->next != NULL) {
        allocation->next->previous = allocation->previous;
    }
    heap->allocatedSize -= allocation->size;
    heap->allocationCount--;
    allocation->magic = 0;
    free(allocation->rawAllocation);
    return true;
}

size_t PlatformHeap_GetAllocatedSize(uint32_t heapId)
{
    return PlatformHeap_IsValidId(heapId) && sHeaps[heapId].active
        ? sHeaps[heapId].allocatedSize
        : 0;
}

size_t PlatformHeap_GetAllocationCount(uint32_t heapId)
{
    return PlatformHeap_IsValidId(heapId) && sHeaps[heapId].active
        ? sHeaps[heapId].allocationCount
        : 0;
}
