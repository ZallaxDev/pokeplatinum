#include <stdio.h>

#include "real_port_smoke.h"

int main(void)
{
    char failure[96];

    if (!RealPortSmoke_Run(failure, sizeof(failure))) {
        fprintf(stderr, "REAL PORT SMOKE FAILED: %s\n", failure);
        return 1;
    }

    puts("REAL PORT SMOKE OK: original scheduler, tile rules, application manager, NARC, RTC");
    return 0;
}
