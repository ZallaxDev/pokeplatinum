#include <stdint.h>
#include <stdlib.h>

#include <nnsys.h>

typedef struct HostHeap {
    NNSiFndHeapHead head;
    u32 capacity;
    u32 used;
} HostHeap;

typedef struct HostBlock {
    HostHeap *heap;
    void *raw;
    u32 size;
} HostBlock;

static HostBlock *GetHostBlock(const void *memory)
{
    return (HostBlock *)memory - 1;
}

NNSFndHeapHandle NNS_FndCreateExpHeapEx(void *startAddress, u32 size, u16 optFlag)
{
    HostHeap *heap = calloc(1, sizeof(*heap));

    (void)optFlag;
    if (heap == NULL) {
        return NNS_FND_HEAP_INVALID_HANDLE;
    }
    heap->head.heapStart = startAddress;
    heap->head.heapEnd = (u8 *)startAddress + size;
    heap->capacity = size;
    return &heap->head;
}

void NNS_FndDestroyExpHeap(NNSFndHeapHandle handle)
{
    free((HostHeap *)handle);
}

void *NNS_FndAllocFromExpHeapEx(NNSFndHeapHandle handle, u32 size, int alignment)
{
    HostHeap *heap = (HostHeap *)handle;
    u32 absoluteAlignment = alignment < 0 ? (u32)-alignment : (u32)alignment;
    size_t allocationSize = sizeof(HostBlock) + size + absoluteAlignment - 1;
    void *raw;
    uintptr_t aligned;
    HostBlock *block;

    if (heap == NULL || size > heap->capacity - heap->used) {
        return NULL;
    }
    raw = malloc(allocationSize);
    if (raw == NULL) {
        return NULL;
    }
    aligned = ((uintptr_t)raw + sizeof(HostBlock) + absoluteAlignment - 1)
        & ~(uintptr_t)(absoluteAlignment - 1);
    block = (HostBlock *)aligned - 1;
    block->heap = heap;
    block->raw = raw;
    block->size = size;
    heap->used += size;
    return (void *)aligned;
}

void NNS_FndFreeToExpHeap(NNSFndHeapHandle handle, void *memory)
{
    HostBlock *block = GetHostBlock(memory);
    HostHeap *heap = (HostHeap *)handle;

    heap->used -= block->size;
    free(block->raw);
}

u32 NNS_FndGetTotalFreeSizeForExpHeap(NNSFndHeapHandle handle)
{
    HostHeap *heap = (HostHeap *)handle;
    return heap->capacity - heap->used;
}

u32 NNS_FndGetSizeForMBlockExpHeap(const void *memory)
{
    return GetHostBlock(memory)->size;
}

u32 NNS_FndResizeForMBlockExpHeap(NNSFndHeapHandle handle, void *memory, u32 size)
{
    HostHeap *heap = (HostHeap *)handle;
    HostBlock *block = GetHostBlock(memory);

    if (size > block->size) {
        return 0;
    }
    heap->used -= block->size - size;
    block->size = size;
    return size;
}

static void *AllocatorAlloc(NNSFndAllocator *allocator, u32 size)
{
    return NNS_FndAllocFromExpHeapEx(allocator->pHeap, size, (int)allocator->heapParam1);
}

static void AllocatorFree(NNSFndAllocator *allocator, void *memory)
{
    NNS_FndFreeToExpHeap(allocator->pHeap, memory);
}

void NNS_FndInitAllocatorForExpHeap(NNSFndAllocator *allocator, NNSFndHeapHandle heap, int alignment)
{
    static const NNSFndAllocatorFunc functions = { AllocatorAlloc, AllocatorFree };

    allocator->pFunc = &functions;
    allocator->pHeap = heap;
    allocator->heapParam1 = (u32)alignment;
    allocator->heapParam2 = 0;
}
