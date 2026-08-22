# NDS API Replacement Map

| NDS API / abstraction | Current users | Purpose | 3DS replacement | Status |
|---|---|---|---|---|
| `NitroMain`, `OS_WaitIrq` | `src/main.c` | process entry and frame wait | `main`, `aptMainLoop`, presentation scheduler | bootstrap implemented; gameplay pending |
| `PAD_Read`, `TP_*` | `src/system.c`, `src/touch_pad.c` | buttons and touch | `PlatformInput` over libctru HID | bootstrap implemented |
| `FS_*` | `src/system.c`, `src/narc.c` | files and NARC streams | `PlatformFile` over RomFS | exact-read bootstrap implemented |
| `CARD_*` | `src/savedata.c` | save flash | transactional `PlatformSave` file | pending |
| `OS_TIMER_3`, timer registers | `src/timer.c` | monotonic timing | platform monotonic ticks | bootstrap scheduler implemented; gameplay pending |
| `RTC_*` | `src/rtc.c` | wall clock | 3DS system time service | pending |
| `GX_*`, `GXS_*`, `G2_*` | graphics and applications | display and 2D engines | renderer commands via Citro2D/Citro3D | pending |
| `NNS_G2d*`, OAM manager | sprite/graphics modules | resource parsing and sprites | portable decoders plus sprite batches | pending |
| `NNS_G3d*`, `G3_*` | field, battle, effects | model rendering | Citro3D backend | pending |
| `NNS_Gfd*` | transfers and graphics | VRAM allocation/upload | explicit GPU resource manager | pending |
| `NNS_Snd*`, `MIC_*` | sound modules | audio and recording | NDSP/conversion; 3DS mic later | pending |
| `OS_AllocFromMainArena*`, `NNS_Fnd*` | `src/system.c`, `src/heap.c` | arenas/heaps | portable heap backend | base implemented; game hierarchy pending |
| `MI_Cpu*`, `MI_Dma*` | widespread | memory operations | `memcpy`/`memset` or backend transfer | pending by call semantics |
| `DC_FlushRange`, `DC_InvalidateRange` | graphics/audio/wireless | device coherency | backend-specific synchronization | pending |
| `FS_LoadOverlay*` | `src/game_overlay.c` | dynamic code modules | static application registry | pending |
| `PM_*`, lid/reset APIs | `src/main.c`, `src/system.c` | power policy | libctru lifecycle or explicit no-op policy | pending |
| `WM_*`, DWC, NitroWiFi | communications modules | networking | explicit offline backend | pending |
