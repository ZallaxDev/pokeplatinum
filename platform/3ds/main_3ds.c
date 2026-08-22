#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>

#include "real_port_smoke.h"

void ErrorHandling_AssertFail(void)
{
    abort();
}

int main(void)
{
    char failure[96];
    BOOL passed;

    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    passed = RealPortSmoke_Run(failure, sizeof(failure));
    printf("Pokemon Platinum real-port bootstrap\n\n");
    printf("Original src/sys_task_manager.c: %s\n", passed ? "PASS" : "FAIL");
    printf("Original src/map_tile_behavior.c: %s\n", passed ? "PASS" : "FAIL");
    printf("Original src/overlay_manager.c: %s\n", passed ? "PASS" : "FAIL");
    if (!passed) {
        printf("\n%s\n", failure);
    }
    printf("\nSTART: exit\n");

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) {
            break;
        }
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return passed ? 0 : 1;
}
