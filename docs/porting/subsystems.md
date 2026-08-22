# Porting Subsystem Classification

Classification describes logic reuse, not whether a file currently compiles without NitroSDK.

| Subsystem | Classification | Representative files | Initial boundary |
|---|---|---|---|
| Pokemon, party, evolution, items | mostly-independent | `src/pokemon.c`, `src/party.c`, `src/evolution.c`, `src/item.c` | isolate resource/time helpers |
| Battle rules and AI | mostly-independent | `src/battle/`, `src/battle/trainer_ai/` | separate presentation, input, sound, networking |
| Script engines | mostly-independent | `src/field_script_context.c`, `src/scrcmd.c`, `src/battle/battle_script.c` | route commands through portable services |
| Application manager | mostly-independent | `src/overlay_manager.c` | replace binary overlay loader |
| Task manager | mostly-independent | `src/sys_task_manager.c` | preserve frame-phase execution |
| Save block layout | platform-independent | `src/savedata/save_table.c`, save component files | retain layouts and checksums |
| Save orchestration | mixed | `src/savedata.c` | split CARD storage from recovery logic |
| Field state, movement, collision | mixed | `src/field_system.c`, `src/player_move.c`, `src/map_matrix.c` | separate renderer/input/filesystem/overlays |
| Input normalization/hit tests | mostly-independent | `include/system.h`, `src/touch_screen.c` | backend populates logical state |
| Input polling/calibration | hardware-dependent | `src/system.c`, `src/touch_pad.c` | libctru HID backend |
| Entry point/frame/interrupts | hardware-dependent | `src/main.c`, `src/system.c` | 3DS lifecycle and fixed logical scheduler |
| Timer | hardware-dependent | `src/timer.c` | monotonic platform clock |
| RTC | mixed | `src/rtc.c` | portable calendar and 3DS acquisition implemented; connect original consumers |
| Files and NARC | mixed | `src/system.c`, `src/narc.c` | portable stream API under NARC parser |
| BG/windows | mixed | `src/bg_window.c` | 4bpp text BG and message-frame composition implemented; connect general layer/window state |
| Graphics resource loading | mixed | `src/graphics.c` | retain formats, replace NNS parsing/upload as needed |
| Sprites/OAM | hardware-dependent | `src/render_oam.c`, `src/sprite_transfer.c` | renderer sprite abstraction |
| 3D renderer | hardware-dependent | `src/g3d_pipeline.c`, `src/easy3d.c` | Citro3D backend and format loaders |
| Particles | mixed | `lib/spl/` | split simulation from GX draw path |
| Audio policy | mixed | `src/sound.c`, `src/sound_playback.c` | retain IDs/state, replace engine |
| SDAT engine/microphone | hardware-dependent | `src/sound_system.c`, `src/sound_chatot.c` | NDSP/conversion and later microphone service |
| Overlay loader | hardware-dependent | `src/game_overlay.c` | static registry/no-op load bookkeeping |
| Boot/cartridge/GBA migration | hardware-dependent | `src/boot.c`, `src/main_menu/gba_migrator.c` | exclude or redesign explicitly |
| Local wireless/WFC | hardware-dependent | `src/wireless_manager.c`, `src/communication_system.c` | offline policy implemented; route original entry points through it |
| Obfuscated late overlays | unknown | `src/unk_*.c`, `src/overlay061/` through `src/overlay117/` | classify per feature before inclusion |
| Host asset tools | mostly-independent | `tools/`, `res/meson.build` | retain as host-side pipeline |
