# Nintendo DS Platform Dependencies

This inventory identifies replacement boundaries; it is not permission to replace APIs mechanically by name.

| Category | Representative current users | Observable purpose | 3DS direction |
|---|---|---|---|
| NitroSDK lifecycle | `src/main.c`, `src/system.c` | startup, frame loop, sleep, reset | libctru lifecycle plus implemented portable init/frame/shutdown boundary |
| GX/G2/BG | `src/gx_layers.c`, `src/bg_window.c` | display modes, tile backgrounds, windows | renderer state interpreted by Citro2D/Citro3D |
| OBJ/OAM | `src/render_oam.c`, `src/sprite*.c` | sprite allocation, transform, priority | logical sprite batches, no physical OAM emulation |
| VRAM/palettes | `src/system.c`, `src/vram_transfer.c`, `src/graphics.c` | texture, tile, model, and palette storage | CPU resources plus explicit GPU uploads |
| 3D GX/NNS G3D | `src/g3d_pipeline.c`, `src/easy3d*.c`, `lib/spl/src/spl_draw.c` | field models, battle objects, particles | model/texture loaders and Citro3D draw backend |
| DMA/cache | `MI_Dma*`, `DC_FlushRange`, `DC_InvalidateRange` users | copies and device coherency | CPU copies or backend-specific GPU/audio synchronization |
| IRQ/VBlank/HBlank | `src/main.c`, `src/system.c`, `src/overlay005/hblank_system.c` | frame phases and scanline effects | frame scheduler; renderer-generated scanline effects |
| OS/timers | `src/timer.c`, `src/sys_task_manager.c` | monotonic time and task execution | libctru clock plus portable scheduler |
| FS/NitroFS | `src/system.c`, `src/narc.c` | file and archive reads | relative `PlatformFile` paths backed by RomFS |
| CARD | `src/savedata.c` | backup flash reads/writes | transactional save file backend |
| RTC | `src/rtc.c` | game clock and daily events | 3DS wall clock plus retained calendar logic |
| Keypad/touch | `src/system.c`, `src/touch_pad.c`, `src/touch_screen.c` | digital input, calibrated touch, hit tests | libctru HID feeding existing logical input model |
| Sound/microphone | `src/sound_system.c`, `src/sound_chatot.c` | SDAT playback and Chatot recording | NDSP pipeline; microphone deferred separately |
| Wireless/WFC | `src/wireless_manager.c`, `src/communication_system.c`, `lib/gds`, `lib/ppwlobby` | local and online communications | explicit offline backend for first playable port |
| ARM7 services | touch, RTC, CARD, sound, power APIs | delegated NDS system services | replace per service; never emulate ARM7 |
| Overlays | `src/game_overlay.c`, `src/overlay_manager.c`, `platinum.us/main.lsf` | code loading and application lifecycle | static link plus application registry/bookkeeping |
| Fixed memory/registers | `src/boot.c`, `src/timer.c`, `include/constants/graphics.h` | ROM header, timers, VRAM and palettes | exclude boot policy; platform time and renderer resources |
| Cache/arenas/heaps | `src/heap.c`, `src/system.c` | allocation hierarchy and direction | portable allocator preserving required semantics |

Notable fixed NDS assumptions include `0x02000000` ARM9 placement, ITCM/DTCM sections in `platinum.us/main.lsf`, ROM header buffers in `src/boot.c`, timer registers in `src/timer.c`, and direct palette/VRAM pointers in graphics code.
