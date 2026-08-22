#include "platform/memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PLATFORM_HEAP_COUNT 128
#define PLATFORM_HEAP_NO_PARENT UINT32_MAX

typedef struct PlatformAllocation PlatformAllocation;
typedef struct PlatformFreeRange PlatformFreeRange;

typedef struct PlatformHeap {
    PlatformAllocation *allocations;
    PlatformFreeRange *freeRanges;
    unsigned char *storage;
    void *parentAllocation;
    size_t capacity;
    size_t allocatedSize;
    size_t allocationCount;
    uint32_t parentHeapId;
    bool ownsStorage;
    bool active;
} PlatformHeap;

struct PlatformAllocation {
    PlatformAllocation *next;
    size_t offset;
    size_t size;
};

struct PlatformFreeRange {
    PlatformFreeRange *next;
    size_t offset;
    size_t size;
};

static PlatformHeap sHeaps[PLATFORM_HEAP_COUNT];

static bool PlatformHeap_IsValidId(uint32_t heapId)
{
    return heapId < PLATFORM_HEAP_COUNT;
}

static bool PlatformHeap_HasChild(uint32_t heapId)
{
    for (uint32_t i = 0; i < PLATFORM_HEAP_COUNT; i++) {
        if (sHeaps[i].active && sHeaps[i].parentHeapId == heapId) {
            return true;
        }
    }
    return false;
}

static bool PlatformHeap_Setup(uint32_t heapId, unsigned char *storage,
    size_t capacity, bool ownsStorage, uint32_t parentHeapId, void *parentAllocation)
{
    PlatformFreeRange *range = malloc(sizeof(*range));

    if (range == NULL) {
        return false;
    }
    range->next = NULL;
    range->offset = 0;
    range->size = capacity;
    sHeaps[heapId].freeRanges = range;
    sHeaps[heapId].storage = storage;
    sHeaps[heapId].parentAllocation = parentAllocation;
    sHeaps[heapId].capacity = capacity;
    sHeaps[heapId].parentHeapId = parentHeapId;
    sHeaps[heapId].ownsStorage = ownsStorage;
    sHeaps[heapId].active = true;
    return true;
}

void PlatformHeap_Init(void)
{
    memset(sHeaps, 0, sizeof(sHeaps));
}

void PlatformHeap_Shutdown(void)
{
    for (uint32_t heapId = 0; heapId < PLATFORM_HEAP_COUNT; heapId++) {
        PlatformAllocation *allocation = sHeaps[heapId].allocations;
        PlatformFreeRange *range = sHeaps[heapId].freeRanges;

        while (allocation != NULL) {
            PlatformAllocation *next = allocation->next;
            free(allocation);
            allocation = next;
        }
        while (range != NULL) {
            PlatformFreeRange *next = range->next;
            free(range);
            range = next;
        }
        if (sHeaps[heapId].ownsStorage) {
            free(sHeaps[heapId].storage);
        }
    }
    memset(sHeaps, 0, sizeof(sHeaps));
}

bool PlatformHeap_Create(uint32_t heapId, size_t capacity)
{
    unsigned char *storage;

    if (!PlatformHeap_IsValidId(heapId) || capacity == 0 || sHeaps[heapId].active) {
        return false;
    }
    storage = malloc(capacity);
    if (storage == NULL) {
        return false;
    }
    if (!PlatformHeap_Setup(heapId, storage, capacity, true,
            PLATFORM_HEAP_NO_PARENT, NULL)) {
        free(storage);
        return false;
    }
    return true;
}

static void *PlatformHeap_AllocInternal(uint32_t heapId, size_t size,
    size_t alignment, bool atEnd)
{
    PlatformHeap *heap;
    PlatformFreeRange *range;
    PlatformFreeRange *selected = NULL;
    PlatformAllocation *allocation;
    uintptr_t selectedAddress = 0;
    size_t selectedOffset = 0;

    if (!PlatformHeap_IsValidId(heapId) || !sHeaps[heapId].active || size == 0
        || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return NULL;
    }
    heap = &sHeaps[heapId];
    for (range = heap->freeRanges; range != NULL; range = range->next) {
        uintptr_t start = (uintptr_t)heap->storage + range->offset;
        uintptr_t address;

        if (range->size < size) {
            continue;
        }
        if (atEnd) {
            address = (start + range->size - size) & ~(uintptr_t)(alignment - 1);
        } else {
            address = (start + alignment - 1) & ~(uintptr_t)(alignment - 1);
        }
        if (address < start || address - start > range->size - size) {
            continue;
        }
        if (selected == NULL || (atEnd && address > selectedAddress)) {
            selected = range;
            selectedAddress = address;
            selectedOffset = address - (uintptr_t)heap->storage;
            if (!atEnd) {
                break;
            }
        }
    }
    if (selected == NULL) {
        return NULL;
    }
    allocation = malloc(sizeof(*allocation));
    if (allocation == NULL) {
        return NULL;
    }
    allocation->offset = selectedOffset;
    allocation->size = size;
    allocation->next = heap->allocations;
    heap->allocations = allocation;

    if (selectedOffset == selected->offset) {
        selected->offset += size;
        selected->size -= size;
    } else {
        size_t suffixOffset = selectedOffset + size;
        size_t suffixSize = selected->offset + selected->size - suffixOffset;

        if (suffixSize != 0) {
            PlatformFreeRange *suffix = malloc(sizeof(*suffix));
            if (suffix == NULL) {
                heap->allocations = allocation->next;
                free(allocation);
                return NULL;
            }
            suffix->next = selected->next;
            suffix->offset = suffixOffset;
            suffix->size = suffixSize;
            selected->next = suffix;
        }
        selected->size = selectedOffset - selected->offset;
    }
    if (selected->size == 0) {
        PlatformFreeRange **link = &heap->freeRanges;
        while (*link != selected) {
            link = &(*link)->next;
        }
        *link = selected->next;
        free(selected);
    }
    heap->allocatedSize += allocation->size;
    heap->allocationCount++;
    return heap->storage + allocation->offset;
}

bool PlatformHeap_CreateChild(uint32_t parentHeapId, uint32_t childHeapId,
    size_t capacity, bool atEnd)
{
    void *storage;

    if (!PlatformHeap_IsValidId(parentHeapId)
        || !PlatformHeap_IsValidId(childHeapId) || !sHeaps[parentHeapId].active
        || sHeaps[childHeapId].active || capacity == 0) {
        return false;
    }
    storage = PlatformHeap_AllocInternal(parentHeapId, capacity, 4, atEnd);
    if (storage == NULL) {
        return false;
    }
    if (!PlatformHeap_Setup(childHeapId, storage, capacity, false,
            parentHeapId, storage)) {
        PlatformHeap_Free(parentHeapId, storage);
        return false;
    }
    return true;
}

bool PlatformHeap_Destroy(uint32_t heapId)
{
    PlatformHeap heap;

    if (!PlatformHeap_IsValidId(heapId) || !sHeaps[heapId].active
        || sHeaps[heapId].allocationCount != 0 || PlatformHeap_HasChild(heapId)) {
        return false;
    }
    heap = sHeaps[heapId];
    free(heap.freeRanges);
    memset(&sHeaps[heapId], 0, sizeof(sHeaps[heapId]));
    if (heap.ownsStorage) {
        free(heap.storage);
        return true;
    }
    return PlatformHeap_Free(heap.parentHeapId, heap.parentAllocation);
}

void *PlatformHeap_Alloc(uint32_t heapId, size_t size, size_t alignment)
{
    return PlatformHeap_AllocInternal(heapId, size, alignment, false);
}

void *PlatformHeap_AllocAtEnd(uint32_t heapId, size_t size, size_t alignment)
{
    return PlatformHeap_AllocInternal(heapId, size, alignment, true);
}

bool PlatformHeap_Free(uint32_t heapId, void *allocationAddress)
{
    PlatformHeap *heap;
    PlatformAllocation **allocationLink;
    PlatformAllocation *allocation;
    PlatformFreeRange **rangeLink;
    PlatformFreeRange *range;

    if (!PlatformHeap_IsValidId(heapId) || allocationAddress == NULL
        || !sHeaps[heapId].active) {
        return false;
    }
    heap = &sHeaps[heapId];
    allocationLink = &heap->allocations;
    while (*allocationLink != NULL
        && heap->storage + (*allocationLink)->offset != allocationAddress) {
        allocationLink = &(*allocationLink)->next;
    }
    if (*allocationLink == NULL) {
        return false;
    }
    allocation = *allocationLink;
    *allocationLink = allocation->next;

    range = malloc(sizeof(*range));
    if (range == NULL) {
        allocation->next = heap->allocations;
        heap->allocations = allocation;
        return false;
    }
    range->offset = allocation->offset;
    range->size = allocation->size;
    rangeLink = &heap->freeRanges;
    while (*rangeLink != NULL && (*rangeLink)->offset < range->offset) {
        rangeLink = &(*rangeLink)->next;
    }
    range->next = *rangeLink;
    *rangeLink = range;
    if (range->next != NULL && range->offset + range->size == range->next->offset) {
        PlatformFreeRange *next = range->next;
        range->size += next->size;
        range->next = next->next;
        free(next);
    }
    if (rangeLink != &heap->freeRanges) {
        PlatformFreeRange *previous = heap->freeRanges;
        while (previous->next != range) {
            previous = previous->next;
        }
        if (previous->offset + previous->size == range->offset) {
            previous->size += range->size;
            previous->next = range->next;
            free(range);
        }
    }
    heap->allocatedSize -= allocation->size;
    heap->allocationCount--;
    free(allocation);
    return true;
}

size_t PlatformHeap_GetCapacity(uint32_t heapId)
{
    return PlatformHeap_IsValidId(heapId) && sHeaps[heapId].active
        ? sHeaps[heapId].capacity : 0;
}

size_t PlatformHeap_GetAllocatedSize(uint32_t heapId)
{
    return PlatformHeap_IsValidId(heapId) && sHeaps[heapId].active
        ? sHeaps[heapId].allocatedSize : 0;
}

size_t PlatformHeap_GetAllocationCount(uint32_t heapId)
{
    return PlatformHeap_IsValidId(heapId) && sHeaps[heapId].active
        ? sHeaps[heapId].allocationCount : 0;
}
