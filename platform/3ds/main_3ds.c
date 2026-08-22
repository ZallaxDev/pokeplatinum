#include <3ds.h>
#include <stdio.h>

#include "platform/debug.h"
#include "platform/filesystem.h"
#include "platform/input.h"
#include "platform/platform.h"
#include "platform/time.h"

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

int main(void)
{
    static unsigned char smokeData[256];
    PlatformInputState input = { 0 };
    PlatformTickScheduler tickScheduler;
    PrintConsole topConsole;
    unsigned long long frame = 0;
    size_t smokeSize = 0;
    const char *lastInput = "NONE";
    bool platformReady = Platform_Init();

    PlatformTickScheduler_Init(&tickScheduler);

    consoleInit(GFX_TOP, &topConsole);
    consoleSelect(&topConsole);
    printf("\x1b[2;6HPOKEMON PLATINUM - NATIVE 3DS");
    printf("\x1b[4;17HBOOT %s", platformReady ? "OK" : "FAILED");
    printf("\x1b[6;14HMilestone BOOT-01");
    printf("\x1b[9;5HButtons and touch are shown below.");
    printf("\x1b[14;4HSTART exits this bootstrap build.");
#if PORT3DS_DEBUG_OVERLAY
    printf("\x1b[15;4HL+R+SELECT toggles debug.");
#endif
    printf("\x1b[17;4HTicks: %10llu", tickScheduler.tickCount);
    printf("\x1b[18;4HCadence:  0.000 Hz");

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

    while (Platform_MainLoop()) {
        unsigned long long elapsedMs;
        unsigned long long rateMilliHz;

        PlatformTickScheduler_Update(&tickScheduler);
        PlatformInput_Update(&input);
        if (input.pressed != 0) {
            lastInput = DescribeInput(input.pressed);
            Debug_Log("Input %s", lastInput);
        }
        if (input.touchPressed) {
            Debug_Log("Touch %u,%u", input.touchX, input.touchY);
            lastInput = "TOUCH";
        }

#if PORT3DS_DEBUG_OVERLAY
        if ((input.held & (GAME_KEY_L | GAME_KEY_R)) == (GAME_KEY_L | GAME_KEY_R)
            && (input.pressed & GAME_KEY_SELECT)) {
            Debug_SetOverlayEnabled(!Debug_IsOverlayEnabled());
        }
#endif

        consoleSelect(&topConsole);
        printf("\x1b[11;8HLast: %-10s", lastInput);
        printf("\x1b[12;8HTouch: %3u,%3u %-4s", input.touchX, input.touchY,
            input.touchHeld ? "DOWN" : "UP");
        elapsedMs = tickScheduler.elapsedNs / 1000000ULL;
        rateMilliHz = elapsedMs == 0 ? 0 : tickScheduler.tickCount * 1000000ULL / elapsedMs;
        printf("\x1b[17;4HTicks: %10llu", tickScheduler.tickCount);
        printf("\x1b[18;4HCadence: %2llu.%03llu Hz", rateMilliHz / 1000ULL,
            rateMilliHz % 1000ULL);
        Debug_Render(frame, lastInput, tickScheduler.tickCount, tickScheduler.elapsedNs);
        Platform_WaitForFrame();
        frame++;

        if (input.pressed & GAME_KEY_START) {
            break;
        }
    }

    Platform_Shutdown();
    return 0;
}
