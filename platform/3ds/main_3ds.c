#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "platform/archive.h"
#include "platform/debug.h"
#include "platform/filesystem.h"
#include "platform/game_application.h"
#include "platform/game_clock.h"
#include "platform/graphics.h"
#include "platform/game_runtime.h"
#include "platform/game_task.h"
#include "platform/input.h"
#include "platform/memory.h"
#include "platform/nitro2d.h"
#include "platform/platform.h"
#include "platform/save.h"
#include "platform/time.h"

#define SAVE_SMOKE_PATH "port3ds-atomic-smoke.bin"
#define SAVE_SMOKE_MAGIC 0x50543344u
#define SAVE_SMOKE_INITIAL_VALUE 0x13579BDFu
#define GENERATED_NARC_PATH "generated/evo.narc"
#define GENERATED_NARC_CAPACITY 32768
#define ICON_NARC_PATH "generated/pl_poke_icon.narc"
#define ICON_NARC_MAX_SIZE (1024 * 1024)
#define TURTWIG_ICON_MEMBER 394
#define TURTWIG_PALETTE_MEMBER 0
#define TURTWIG_PALETTE_BANK 1

typedef struct SaveSmokeRecord {
    uint32_t magic;
    uint32_t value;
    uint32_t checksum;
} SaveSmokeRecord;

static unsigned char sGeneratedNarc[GENERATED_NARC_CAPACITY];
static size_t sGeneratedNarcSize;
static unsigned char *sIconNarc;
static uint32_t sIconPixels[32 * 32];

enum BootstrapTaskPhase {
    BOOTSTRAP_TASK_MAIN,
    BOOTSTRAP_TASK_FRAME_BOUNDARY,
    BOOTSTRAP_TASK_PRINT,
    BOOTSTRAP_TASK_POST_FRAME,
    BOOTSTRAP_TASK_PHASE_COUNT,
};

enum BootstrapApplicationId {
    BOOTSTRAP_APPLICATION_ALPHA = 1,
    BOOTSTRAP_APPLICATION_BETA = 2,
};

typedef struct BootstrapGameState BootstrapGameState;

typedef struct BootstrapTaskContext {
    BootstrapGameState *game;
    enum BootstrapTaskPhase phase;
} BootstrapTaskContext;

struct BootstrapGameState {
    uint64_t updateCount;
    uint64_t taskCounts[BOOTSTRAP_TASK_PHASE_COUNT];
    GameTaskManager taskManagers[BOOTSTRAP_TASK_PHASE_COUNT];
    BootstrapTaskContext taskContexts[BOOTSTRAP_TASK_PHASE_COUNT];
    GameApplicationRegistry applicationRegistry;
    GameApplicationRunner applicationRunner;
    uint64_t applicationSwitchCount;
    enum BootstrapTaskPhase expectedPhase;
    bool taskOrderValid;
    bool taskSmokeLogged;
};

static bool BootstrapApplication_Init(GameApplicationRunner *runner,
    void *context, int *state)
{
    (void)context;
    (void)state;
    Debug_Log("APP %s INIT", GameApplicationRunner_GetCurrentName(runner));
    return true;
}

static bool BootstrapApplication_Main(GameApplicationRunner *runner,
    void *context, int *state)
{
    (void)context;
    if (*state == 0) {
        Debug_Log("APP %s MAIN", GameApplicationRunner_GetCurrentName(runner));
    }
    (*state)++;
    return *state >= 60;
}

static bool BootstrapApplication_Exit(GameApplicationRunner *runner,
    void *context, int *state)
{
    BootstrapGameState *game = context;
    uint32_t currentId = GameApplicationRunner_GetCurrentId(runner);
    uint32_t nextId = currentId == BOOTSTRAP_APPLICATION_ALPHA
        ? BOOTSTRAP_APPLICATION_BETA
        : BOOTSTRAP_APPLICATION_ALPHA;
    (void)state;

    Debug_Log("APP %s EXIT", GameApplicationRunner_GetCurrentName(runner));
    if (!GameApplicationRunner_Queue(runner, nextId)) {
        return false;
    }
    game->applicationSwitchCount++;
    return true;
}

static const GameApplicationTemplate sBootstrapApplications[] = {
    {
        BOOTSTRAP_APPLICATION_ALPHA,
        "ALPHA",
        BootstrapApplication_Init,
        BootstrapApplication_Main,
        BootstrapApplication_Exit,
    },
    {
        BOOTSTRAP_APPLICATION_BETA,
        "BETA",
        BootstrapApplication_Init,
        BootstrapApplication_Main,
        BootstrapApplication_Exit,
    },
};

static void BootstrapTask_Run(GameTask *task, void *context)
{
    BootstrapTaskContext *taskContext = context;
    BootstrapGameState *game = taskContext->game;
    (void)task;

    if (taskContext->phase != game->expectedPhase) {
        game->taskOrderValid = false;
        return;
    }
    game->taskCounts[taskContext->phase]++;
    game->expectedPhase = (enum BootstrapTaskPhase)(taskContext->phase + 1);
}

static bool BootstrapGame_Init(void *context)
{
    BootstrapGameState *state = context;
    state->updateCount = 0;
    state->expectedPhase = BOOTSTRAP_TASK_MAIN;
    state->taskOrderValid = true;
    state->taskSmokeLogged = false;
    state->applicationSwitchCount = 0;
    for (unsigned int i = 0; i < BOOTSTRAP_TASK_PHASE_COUNT; i++) {
        state->taskContexts[i].game = state;
        state->taskContexts[i].phase = (enum BootstrapTaskPhase)i;
        if (!GameTaskManager_Init(&state->taskManagers[i], 4)
            || GameTaskManager_Add(&state->taskManagers[i], BootstrapTask_Run,
                &state->taskContexts[i], 0) == NULL) {
            return false;
        }
    }
    if (!GameApplicationRegistry_Init(&state->applicationRegistry,
            sBootstrapApplications,
            sizeof(sBootstrapApplications) / sizeof(sBootstrapApplications[0]))
        || !GameApplicationRunner_Init(&state->applicationRunner,
            &state->applicationRegistry, state)
        || !GameApplicationRunner_Queue(&state->applicationRunner,
            BOOTSTRAP_APPLICATION_ALPHA)) {
        return false;
    }
    Debug_Log("GAME INIT OK");
    return true;
}

static bool BootstrapGame_Frame(void *context)
{
    BootstrapGameState *state = context;
    if (!GameApplicationRunner_RunFrame(&state->applicationRunner)) {
        return false;
    }
    state->expectedPhase = BOOTSTRAP_TASK_MAIN;
    for (unsigned int i = 0; i < BOOTSTRAP_TASK_PHASE_COUNT; i++) {
        if (!GameTaskManager_Execute(&state->taskManagers[i])) {
            return false;
        }
    }
    if (!state->taskOrderValid || state->expectedPhase != BOOTSTRAP_TASK_PHASE_COUNT) {
        return false;
    }
    state->updateCount++;
    if (!state->taskSmokeLogged) {
        Debug_Log("TASK PHASES M>B>P>A OK");
        state->taskSmokeLogged = true;
    }
    return true;
}

static void BootstrapGame_Shutdown(void *context)
{
    BootstrapGameState *state = context;
    Debug_Log("GAME SHUTDOWN %llu", (unsigned long long)state->updateCount);
}

static const char *DescribeInput(uint32_t keys)
{
    if (keys & GAME_KEY_A) return "A";
    if (keys & GAME_KEY_B) return "B";
    if (keys & GAME_KEY_X) return "X";
    if (keys & GAME_KEY_Y) return "Y";
    if (keys & GAME_KEY_L) return "L";
    if (keys & GAME_KEY_R) return "R";
    if (keys & GAME_KEY_START) return "START";
    if (keys & GAME_KEY_SELECT) return "SELECT";
    if (keys & GAME_KEY_UP) return "UP";
    if (keys & GAME_KEY_DOWN) return "DOWN";
    if (keys & GAME_KEY_LEFT) return "LEFT";
    if (keys & GAME_KEY_RIGHT) return "RIGHT";
    return "NONE";
}

static uint32_t Fnv1a(const unsigned char *data, size_t size)
{
    uint32_t hash = 2166136261u;

    for (size_t i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static bool RunHeapSmokeTest(void)
{
    static const struct {
        uint32_t heapId;
        size_t capacity;
        size_t size;
        size_t alignment;
        unsigned char pattern;
    } tests[] = {
        { PLATFORM_HEAP_SYSTEM, 4096, 257, 8, 0x31 },
        { PLATFORM_HEAP_DEBUG, 2048, 193, 32, 0xA5 },
        { PLATFORM_HEAP_APPLICATION, 8192, 1025, 64, 0x5A },
    };
    void *allocations[sizeof(tests) / sizeof(tests[0])] = { 0 };
    bool passed = true;

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        passed &= PlatformHeap_Create(tests[i].heapId, tests[i].capacity);
        allocations[i] = PlatformHeap_Alloc(tests[i].heapId, tests[i].size, tests[i].alignment);
        passed = passed && allocations[i] != NULL
            && ((uintptr_t)allocations[i] % tests[i].alignment) == 0;
        if (allocations[i] != NULL) {
            memset(allocations[i], tests[i].pattern, tests[i].size);
            const unsigned char *bytes = allocations[i];
            passed = passed && bytes[0] == tests[i].pattern
                && bytes[tests[i].size / 2] == tests[i].pattern
                && bytes[tests[i].size - 1] == tests[i].pattern;
        }
        passed = passed
            && PlatformHeap_GetAllocatedSize(tests[i].heapId) == tests[i].size
            && PlatformHeap_GetAllocationCount(tests[i].heapId) == 1;
    }

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        passed &= PlatformHeap_Free(tests[i].heapId, allocations[i]);
        passed = passed && PlatformHeap_GetAllocatedSize(tests[i].heapId) == 0
            && PlatformHeap_GetAllocationCount(tests[i].heapId) == 0;
        passed &= PlatformHeap_Destroy(tests[i].heapId);
    }
    return passed;
}

static bool RunHeapHierarchySmokeTest(void)
{
    void *low = NULL;
    void *high = NULL;
    bool passed = PlatformHeap_Create(PLATFORM_HEAP_SYSTEM, 8192)
        && PlatformHeap_CreateChild(PLATFORM_HEAP_SYSTEM, PLATFORM_HEAP_DEBUG,
            2048, false)
        && PlatformHeap_CreateChild(PLATFORM_HEAP_SYSTEM, PLATFORM_HEAP_APPLICATION,
            2048, true);

    if (passed) {
        low = PlatformHeap_Alloc(PLATFORM_HEAP_DEBUG, 128, 32);
        high = PlatformHeap_AllocAtEnd(PLATFORM_HEAP_DEBUG, 128, 64);
        passed = low != NULL && high != NULL
            && (uintptr_t)low % 32 == 0 && (uintptr_t)high % 64 == 0
            && (uintptr_t)low < (uintptr_t)high
            && PlatformHeap_GetAllocatedSize(PLATFORM_HEAP_SYSTEM) == 4096;
    }
    if (low != NULL) {
        passed &= PlatformHeap_Free(PLATFORM_HEAP_DEBUG, low);
    }
    if (high != NULL) {
        passed &= PlatformHeap_Free(PLATFORM_HEAP_DEBUG, high);
    }
    if (PlatformHeap_GetCapacity(PLATFORM_HEAP_DEBUG) != 0) {
        passed &= PlatformHeap_Destroy(PLATFORM_HEAP_DEBUG);
    }
    if (PlatformHeap_GetCapacity(PLATFORM_HEAP_APPLICATION) != 0) {
        passed &= PlatformHeap_Destroy(PLATFORM_HEAP_APPLICATION);
    }
    if (PlatformHeap_GetCapacity(PLATFORM_HEAP_SYSTEM) != 0) {
        passed &= PlatformHeap_Destroy(PLATFORM_HEAP_SYSTEM);
    }
    return passed;
}

static SaveSmokeRecord MakeSaveSmokeRecord(uint32_t value)
{
    SaveSmokeRecord record = {
        .magic = SAVE_SMOKE_MAGIC,
        .value = value,
        .checksum = SAVE_SMOKE_MAGIC ^ value ^ 0xFFFFFFFFu,
    };
    return record;
}

static bool LoadSaveSmokeRecord(SaveSmokeRecord *record)
{
    if (PlatformSave_Read(SAVE_SMOKE_PATH, record, sizeof(*record))) {
        return record->magic == SAVE_SMOKE_MAGIC
            && record->checksum == (record->magic ^ record->value ^ 0xFFFFFFFFu);
    }

    *record = MakeSaveSmokeRecord(SAVE_SMOKE_INITIAL_VALUE);
    return PlatformSave_WriteAtomic(SAVE_SMOKE_PATH, record, sizeof(*record));
}

static bool LoadGeneratedNarc(void)
{
    size_t narcSize;
    uint32_t checksum;

    if (!PlatformFile_GetSize(GENERATED_NARC_PATH, &narcSize)
        || narcSize == 0 || narcSize > sizeof(sGeneratedNarc)) {
        return false;
    }
    if (!PlatformFile_Read(GENERATED_NARC_PATH, sGeneratedNarc, narcSize)) {
        return false;
    }
    checksum = Fnv1a(sGeneratedNarc, narcSize);
    sGeneratedNarcSize = narcSize;
    Debug_Log("NARC evo %u bytes", (unsigned int)narcSize);
    Debug_Log("NARC FNV-1a %08lx", (unsigned long)checksum);
    Debug_Log("NARC EXPORT OK");
    return true;
}

static bool InspectGeneratedNarc(void)
{
    PlatformNarc archive;
    const void *member;
    size_t firstSize;
    size_t middleSize;
    size_t lastSize;
    uint16_t count;

    if (!PlatformNarc_Open(&archive, sGeneratedNarc, sGeneratedNarcSize)) {
        return false;
    }
    count = PlatformNarc_GetMemberCount(&archive);
    if (count == 0
        || !PlatformNarc_GetMember(&archive, 0, &member, &firstSize)
        || !PlatformNarc_GetMember(&archive, count / 2, &member, &middleSize)
        || !PlatformNarc_GetMember(&archive, count - 1, &member, &lastSize)) {
        return false;
    }
    Debug_Log("NARC members %u", count);
    Debug_Log("NARC sizes %u/%u/%u", (unsigned int)firstSize,
        (unsigned int)middleSize, (unsigned int)lastSize);
    Debug_Log("NARC PARSE OK");
    return true;
}

static bool LoadTurtwigIcon(void)
{
    PlatformNarc archive;
    PlatformNcgr image;
    PlatformNclr palette;
    const void *imageData;
    const void *paletteData;
    size_t archiveSize;
    size_t imageSize;
    size_t paletteSize;

    if (!PlatformFile_GetSize(ICON_NARC_PATH, &archiveSize)
        || archiveSize == 0 || archiveSize > ICON_NARC_MAX_SIZE) {
        Debug_Error("ICON archive size failed");
        return false;
    }
    if (!PlatformHeap_Create(PLATFORM_HEAP_APPLICATION, archiveSize)) {
        Debug_Error("ICON heap create failed");
        return false;
    }
    sIconNarc = PlatformHeap_Alloc(PLATFORM_HEAP_APPLICATION, archiveSize, 8);
    if (sIconNarc == NULL || !PlatformFile_Read(ICON_NARC_PATH, sIconNarc, archiveSize)) {
        Debug_Error("ICON archive read failed");
        return false;
    }
    if (!PlatformNarc_Open(&archive, sIconNarc, archiveSize)
        || !PlatformNarc_GetMember(&archive, TURTWIG_PALETTE_MEMBER,
            &paletteData, &paletteSize)
        || !PlatformNarc_GetMember(&archive, TURTWIG_ICON_MEMBER, &imageData, &imageSize)) {
        Debug_Error("ICON NARC parse failed");
        return false;
    }
    if (!PlatformNclr_Open(&palette, paletteData, paletteSize)
        || !PlatformNcgr_Open(&image, imageData, imageSize)) {
        Debug_Error("ICON NCGR/NCLR parse failed");
        return false;
    }
    if (!PlatformNitro2D_DecodeTiles4Bpp(&image, &palette,
            TURTWIG_PALETTE_BANK, 0, 32, 32, sIconPixels,
            sizeof(sIconPixels) / sizeof(sIconPixels[0]))) {
        Debug_Error("ICON tile decode failed");
        return false;
    }
    if (!PlatformGraphics_SetSpriteTexture(sIconPixels, 32, 32)) {
        Debug_Error("ICON texture upload failed");
        return false;
    }

    Debug_Log("TURTWIG NCGR %u bytes", (unsigned int)imageSize);
    Debug_Log("TURTWIG NCLR %u colors", (unsigned int)palette.colorCount);
    Debug_Log("PLATINUM ASSET OK");
    return true;
}

static bool BuildLogicalGrid(PlatformScreen screen)
{
    uint32_t *pixels = PlatformGraphics_GetLogicalPixels(screen);
    uint32_t base = screen == PLATFORM_SCREEN_TOP
        ? PLATFORM_RGBA(12, 42, 70, 255)
        : PLATFORM_RGBA(66, 24, 56, 255);
    uint32_t alternate = screen == PLATFORM_SCREEN_TOP
        ? PLATFORM_RGBA(18, 52, 82, 255)
        : PLATFORM_RGBA(78, 30, 68, 255);
    uint32_t grid = screen == PLATFORM_SCREEN_TOP
        ? PLATFORM_RGBA(42, 82, 110, 255)
        : PLATFORM_RGBA(108, 48, 92, 255);
    uint32_t border = screen == PLATFORM_SCREEN_TOP
        ? PLATFORM_RGBA(100, 220, 255, 255)
        : PLATFORM_RGBA(255, 120, 205, 255);

    if (pixels == NULL) {
        return false;
    }
    for (unsigned int y = 0; y < PLATFORM_LOGICAL_HEIGHT; y++) {
        for (unsigned int x = 0; x < PLATFORM_LOGICAL_WIDTH; x++) {
            bool atBorder = x < 2 || y < 2
                || x >= PLATFORM_LOGICAL_WIDTH - 2 || y >= PLATFORM_LOGICAL_HEIGHT - 2;
            bool atGrid = (x % 16) == 0 || (y % 16) == 0;
            bool checker = ((x / 16) + (y / 16)) & 1;
            pixels[y * PLATFORM_LOGICAL_WIDTH + x] = atBorder
                ? border
                : atGrid ? grid : checker ? alternate : base;
        }
    }
    return PlatformGraphics_UploadLogicalSurface(screen);
}

static void RenderBootstrapScreen(bool platformReady, const PlatformInputState *input,
    const char *lastInput, const PlatformTickScheduler *tickScheduler,
    const GameRuntime *gameRuntime, const BootstrapGameState *gameState,
    const GameClockDateTime *clock, bool heapHierarchyReady)
{
    unsigned long long elapsedMs = tickScheduler->elapsedNs / 1000000ULL;
    unsigned long long rateMilliHz = elapsedMs == 0
        ? 0
        : tickScheduler->tickCount * 1000000ULL / elapsedMs;

    PlatformGraphics_BeginScreen(PLATFORM_SCREEN_TOP);
    PlatformGraphics_DrawText(35.0f, 18.0f, 0.65f, PLATFORM_RGBA(120, 220, 255, 255),
        "POKEMON PLATINUM - NATIVE 3DS");
    PlatformGraphics_DrawText(155.0f, 55.0f, 0.55f,
        platformReady ? PLATFORM_RGBA(120, 255, 170, 255) : PLATFORM_RGBA(255, 100, 100, 255),
        "BOOT %s", platformReady ? "OK" : "FAILED");
    PlatformGraphics_DrawText(127.0f, 82.0f, 0.48f, PLATFORM_RGBA(220, 230, 255, 255),
        "Milestone BOOT-01");
    PlatformGraphics_DrawText(52.0f, 112.0f, 0.42f, PLATFORM_RGBA(255, 255, 255, 255),
        "Last: %-10s  Touch: %3u,%3u %s", lastInput, input->touchX, input->touchY,
        input->touchHeld ? "DOWN" : "UP");
    PlatformGraphics_DrawText(52.0f, 125.0f, 0.34f, PLATFORM_RGBA(255, 225, 150, 255),
        "App: %s  switches: %llu",
        GameApplicationRunner_GetCurrentName(&gameState->applicationRunner),
        (unsigned long long)gameState->applicationSwitchCount);
    PlatformGraphics_DrawText(52.0f, 138.0f, 0.42f, PLATFORM_RGBA(255, 255, 255, 255),
        "Game: %llu  Ticks: %llu", (unsigned long long)GameRuntime_GetFrameCount(gameRuntime),
        tickScheduler->tickCount);
    PlatformGraphics_DrawText(52.0f, 157.0f, 0.42f, PLATFORM_RGBA(255, 255, 255, 255),
        "Cadence: %llu.%03llu Hz", rateMilliHz / 1000ULL, rateMilliHz % 1000ULL);
    PlatformGraphics_DrawText(52.0f, 173.0f, 0.38f, PLATFORM_RGBA(180, 240, 210, 255),
        "RTC: %04u-%02u-%02u %02u:%02u:%02u",
        clock->year, clock->month, clock->day,
        clock->hour, clock->minute, clock->second);
    PlatformGraphics_DrawText(52.0f, 189.0f, 0.34f,
        heapHierarchyReady ? PLATFORM_RGBA(180, 240, 210, 255)
                           : PLATFORM_RGBA(255, 120, 120, 255),
        "Heap hierarchy: %s", heapHierarchyReady ? "LOW/HIGH OK" : "FAILED");
    PlatformGraphics_DrawText(52.0f, 96.0f, 0.38f, PLATFORM_RGBA(180, 240, 210, 255),
        "M/B/P/A %llu/%llu/%llu/%llu %s",
        (unsigned long long)gameState->taskCounts[BOOTSTRAP_TASK_MAIN],
        (unsigned long long)gameState->taskCounts[BOOTSTRAP_TASK_FRAME_BOUNDARY],
        (unsigned long long)gameState->taskCounts[BOOTSTRAP_TASK_PRINT],
        (unsigned long long)gameState->taskCounts[BOOTSTRAP_TASK_POST_FRAME],
        gameState->taskOrderValid ? "OK" : "BAD");
    PlatformGraphics_DrawText(128.0f, 215.0f, 0.34f, PLATFORM_RGBA(190, 215, 235, 255),
        "TURTWIG x4   ALPHA: #4");
    PlatformGraphics_DrawText(83.0f, 228.0f, 0.28f, PLATFORM_RGBA(180, 200, 220, 255),
#if PORT3DS_DEBUG_OVERLAY
        "START exits  |  L+R+SELECT toggles debug");
#else
        "START exits this bootstrap build");
#endif
}

int main(void)
{
    static unsigned char smokeData[256];
    PlatformInputState input = { 0 };
    PlatformTickScheduler tickScheduler;
    static BootstrapGameState gameState;
    GameRuntime gameRuntime;
    const GameRuntimeHooks gameHooks = {
        .init = BootstrapGame_Init,
        .frame = BootstrapGame_Frame,
        .shutdown = BootstrapGame_Shutdown,
    };
    SaveSmokeRecord saveRecord;
    GameClockDateTime clock = { 0 };
    unsigned long long frame = 0;
    size_t smokeSize = 0;
    const char *lastInput = "NONE";
    bool heapHierarchyReady;
    bool platformReady = Platform_Init();

    if (platformReady && GameClock_Read(&clock)) {
        Debug_Log("RTC %04u-%02u-%02u %02u:%02u:%02u", clock.year,
            clock.month, clock.day, clock.hour, clock.minute, clock.second);
        Debug_Log("RTC ACQUISITION OK");
    } else {
        Debug_Error("RTC ACQUISITION FAILED");
        platformReady = false;
    }

    if (BuildLogicalGrid(PLATFORM_SCREEN_TOP)
        && BuildLogicalGrid(PLATFORM_SCREEN_BOTTOM)) {
        Debug_Log("SURFACE 256x192 -> 320x240");
        Debug_Log("SURFACE TEST OK");
    } else {
        Debug_Error("SURFACE TEST FAILED");
    }
    if (platformReady) {
        Debug_Log("SPRITE TEST 4 + ALPHA");
        Debug_Log("SPRITE TEST OK");
    }

#if PORT3DS_FATAL_SMOKE
    Debug_Fatal("BOOT", "main", "deliberate fatal smoke test");
#endif

    if (platformReady && PlatformFile_GetSize("port3ds-smoke.txt", &smokeSize)
        && smokeSize <= sizeof(smokeData)
        && PlatformFile_Read("port3ds-smoke.txt", smokeData, smokeSize)) {
        Debug_Log("Loaded port3ds-smoke.txt");
        Debug_Log("Size %u, FNV-1a %08lx", (unsigned int)smokeSize,
            (unsigned long)Fnv1a(smokeData, smokeSize));
    } else {
        Debug_Error("RomFS smoke read failed");
    }
    if (RunHeapSmokeTest()) {
        Debug_Log("HEAP TEST OK");
    } else {
        Debug_Error("HEAP TEST FAILED");
    }
    heapHierarchyReady = RunHeapHierarchySmokeTest();
    if (heapHierarchyReady) {
        Debug_Log("HEAP HIERARCHY LOW/HIGH OK");
    } else {
        Debug_Error("HEAP HIERARCHY FAILED");
    }
    if (!LoadTurtwigIcon()) {
        Debug_Error("PLATINUM ASSET FAILED");
    }
    if (LoadSaveSmokeRecord(&saveRecord)) {
        Debug_Log("SAVE VALUE %08lx", (unsigned long)saveRecord.value);
        if (PlatformSave_HasStagedWrite(SAVE_SMOKE_PATH)) {
            Debug_Log("SAVE STAGED DATA IGNORED");
        }
    } else {
        Debug_Error("SAVE TEST FAILED");
    }
    if (!LoadGeneratedNarc()) {
        Debug_Error("GENERATED NARC FAILED");
    } else if (!InspectGeneratedNarc()) {
        Debug_Error("NARC PARSE FAILED");
    }
    if (!GameRuntime_Init(&gameRuntime, &gameHooks, &gameState)) {
        Debug_Error("GAME INIT FAILED");
        platformReady = false;
    }
    PlatformTickScheduler_Init(&tickScheduler);

    while (Platform_MainLoop()) {
        uint32_t dueTicks = PlatformTickScheduler_Update(&tickScheduler);

        for (uint32_t i = 0; i < dueTicks && GameRuntime_IsRunning(&gameRuntime); i++) {
            if (!GameRuntime_RunFrame(&gameRuntime)) {
                Debug_Error("GAME FRAME FAILED");
                break;
            }
        }
        PlatformInput_Update(&input);
        if (input.pressed != 0) {
            lastInput = DescribeInput(input.pressed);
            Debug_Log("Input %s", lastInput);
        }
        if (input.touchPressed) {
            Debug_Log("Touch %u,%u", input.touchX, input.touchY);
            lastInput = "TOUCH";
        }
        if (input.pressed & GAME_KEY_X) {
            SaveSmokeRecord candidate = MakeSaveSmokeRecord(saveRecord.value ^ 0xFFFFFFFFu);
            if (PlatformSave_Stage(SAVE_SMOKE_PATH, &candidate, sizeof(candidate))) {
                Debug_Log("SAVE STAGED %08lx", (unsigned long)candidate.value);
            } else {
                Debug_Error("SAVE STAGE FAILED");
            }
        }
        if (input.pressed & GAME_KEY_Y) {
            SaveSmokeRecord candidate = MakeSaveSmokeRecord(saveRecord.value ^ 0xFFFFFFFFu);
            if (PlatformSave_WriteAtomic(SAVE_SMOKE_PATH, &candidate, sizeof(candidate))) {
                saveRecord = candidate;
                Debug_Log("SAVE COMMIT %08lx", (unsigned long)saveRecord.value);
            } else {
                Debug_Error("SAVE COMMIT FAILED");
            }
        }

#if PORT3DS_DEBUG_OVERLAY
        if ((input.held & (GAME_KEY_L | GAME_KEY_R)) == (GAME_KEY_L | GAME_KEY_R)
            && (input.pressed & GAME_KEY_SELECT)) {
            Debug_SetOverlayEnabled(!Debug_IsOverlayEnabled());
        }
#endif

        PlatformGraphics_BeginFrame();
        if (!GameClock_Read(&clock)) {
            Debug_Error("RTC UPDATE FAILED");
        }
        PlatformGraphics_PresentLogicalSurface(PLATFORM_SCREEN_TOP);
        PlatformGraphics_DrawSpriteTest();
        RenderBootstrapScreen(platformReady, &input, lastInput, &tickScheduler,
            &gameRuntime, &gameState, &clock, heapHierarchyReady);
        PlatformGraphics_PresentLogicalSurface(PLATFORM_SCREEN_BOTTOM);
        Debug_Render(frame, lastInput, tickScheduler.tickCount, tickScheduler.elapsedNs);
        Platform_WaitForFrame();
        frame++;

        if (input.pressed & GAME_KEY_START) {
            break;
        }
    }

    GameRuntime_Shutdown(&gameRuntime);
    Platform_Shutdown();
    return 0;
}
