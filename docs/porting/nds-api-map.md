# NDS API Replacement Map

| NDS API / abstraction | Current users | Purpose | 3DS replacement | Status |
|---|---|---|---|---|
| `NitroMain`, `OS_WaitIrq` | `src/main.c` | process entry and frame wait | `main`, `aptMainLoop`, game lifecycle, presentation scheduler | init/frame/shutdown boundary implemented; gameplay hooks pending |
| `PAD_Read`, `TP_*` | `src/system.c`, `src/touch_pad.c` | buttons and touch | `PlatformInput` over libctru HID | bootstrap implemented |
| `FS_*` | `src/system.c`, `src/narc.c` | files and NARC streams | `PlatformFile` over RomFS | exact-read and portable NARC parser implemented |
| `CARD_*` | `src/savedata.c` | save flash | transactional `PlatformSave` file | storage base implemented; serializer pending |
| `OS_TIMER_3`, timer registers | `src/timer.c` | monotonic timing | platform monotonic ticks | bootstrap scheduler implemented; gameplay pending |
| `SysTaskManager_*`, `SysTask_*` | `src/sys_task_manager.c`, widespread users | priority queues across frame phases | fixed-capacity portable task manager | priority/mutation semantics and four queues implemented; users pending |
| `RTC_*` | `src/rtc.c` | wall clock | `osGetTime` plus portable Gregorian conversion | acquisition and calendar base implemented; original callers pending |
| `GX_*`, `GXS_*`, `G2_*` | graphics and applications | display and 2D engines | renderer commands via Citro2D/Citro3D | physical targets and logical screen surfaces implemented; game renderer pending |
| `NNS_G2d*`, OAM manager | sprite/graphics modules | resource parsing and sprites | portable decoders plus sprite batches | 4bpp NCGR/NCLR icon subset and sprite submission implemented; broader formats pending |
| `NNS_G3d*`, `G3_*` | field, battle, effects | model rendering | Citro3D backend | pending |
| `NNS_Gfd*` | transfers and graphics | VRAM allocation/upload | explicit GPU resource manager | pending |
| `NNS_Snd*`, `MIC_*` | sound modules | audio and recording | NDSP/conversion; 3DS mic later | pending |
| `OS_AllocFromMainArena*`, `NNS_Fnd*` | `src/system.c`, `src/heap.c` | arenas/heaps | contiguous portable arenas | hierarchy, low/high allocation, free and reuse implemented; callers pending |
| `MI_Cpu*`, `MI_Dma*` | widespread | memory operations | `memcpy`/`memset` or backend transfer | pending by call semantics |
| `DC_FlushRange`, `DC_InvalidateRange` | graphics/audio/wireless | device coherency | backend-specific synchronization | pending |
| `FS_LoadOverlay*` | `src/game_overlay.c` | dynamic code modules | static application registry | registry and multiframe init/main/exit runner implemented; real templates pending |
| `PM_*`, lid/reset APIs | `src/main.c`, `src/system.c` | power policy | libctru lifecycle or explicit no-op policy | pending |
| `WM_*`, DWC, NitroWiFi | communications modules | networking | synchronous offline backend | local/WFC/GTS/Mystery Gift probes implemented; menu callers pending |
