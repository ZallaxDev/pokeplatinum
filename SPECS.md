# Pokemon Platinum Native 3DS Port Specification

## Project Target

- Target: Nintendo 3DS family, ARM11 userland, initial `.3dsx` distribution.
- Language: C11 for new platform code; C++ only where a concrete dependency requires it.
- Toolchain: current supported devkitARM and libctru from devkitPro `3ds-dev`.
- Graphics: Citro2D for suitable 2D composition and Citro3D for 3D or lower-level GPU work.
- Audio: NDSP behind a game-facing audio interface.
- Assets: 3DS RomFS, populated only from resources already produced or legally expected by this repository.
- Compatibility goal: functional game behavior, not binary identity with the NDS ROM.

## Baseline

- Upstream commit: `6b2e9a5bf94c714ef8ef7cda5be06c0ccee01c70`.
- NDS baseline: Rev 1 `build/pokeplatinum.us.nds`.
- Expected NDS SHA-1: `0862ec35b24de5c7e2dcb88c9eea0873110d755c`.
- Host recorded on 2026-08-22: CachyOS, Arch family, Linux `7.2.0-1-cachyos`, x86_64.
- Host compiler recorded: GCC 16.2.1; Python 3.14.7.
- Toolchain: devkitARM r68 (GCC 16.1.0), libctru 2.7.0, Citro2D 1.7.0, Citro3D 1.7.1, and 3dstools 1.3.1.

## Supported Hardware

Old 3DS, Old 3DS XL, New 3DS, New 3DS XL, New 2DS XL, and 2DS are targets. The base port must not require New 3DS-only CPU, memory, C-Stick, or rendering features.

## Build Isolation

- The existing Meson/Metroskrew build remains the authoritative NDS baseline.
- `make 3ds DEBUG=1` delegates to `platform/3ds/Makefile` and uses an explicit source allowlist.
- 3DS objects use mode-specific `build-3ds-debug/` and `build-3ds-release/` directories; deliverables use `build/pokeplatinum-3ds.{elf,3dsx}`.
- New code must not recursively compile all of `src/`. Gameplay units enter the allowlist only after their dependencies are understood.
- Debug builds define `PORT3DS_DEBUG=1` and `PORT3DS_DEBUG_OVERLAY=1`; release builds set both to zero.

## Architecture

Game logic calls platform interfaces for lifecycle, input, time, files, save, memory, graphics, audio, and diagnostics. libctru/Citro2D/Citro3D/NDSP calls stay in `platform/3ds/` or a narrow integration boundary. Repeated `#ifdef __3DS__` branches in gameplay code are not accepted.

The NDS `NitroMain` loop is represented on the portable side by explicit game initialization, one logical frame, and idempotent shutdown hooks. The 3DS entry point owns `aptMainLoop()`, platform events, presentation, and controlled development exit; each scheduler tick invokes exactly one game frame. The bootstrap hooks currently count frames, and later runtime units replace their bodies without taking ownership of the platform loop.

The portable task manager preserves ascending numeric priority, stable ordering for equal priorities, deferred first execution for tasks inserted at or after the current priority, and safe deletion during callbacks. Separate main, frame-boundary, print, and post-frame queues execute in that order for each logical game frame. Fixed-capacity task storage replaces implicit arena allocation at this boundary.

## Screen And Coordinates

- Game top logical surface: 256x192.
- Game bottom logical surface: 256x192.
- Physical top: 400x240. Present the game at 320x240, centered with 40-pixel side bars.
- Physical bottom: 320x240. Present the game at 320x240.
- Gameplay continues to use DS logical coordinates. Scaling belongs to the renderer backend.
- Touch input is converted from 320x240 physical coordinates to 256x192 logical coordinates.
- Wider gameplay viewport and stereoscopic output are explicitly outside this stage.

## Input

`PlatformInputState` exposes held, pressed, and released abstract key masks plus logical touch state. Game code does not call `hidKeys*` directly.

- A/B/X/Y/L/R/START/SELECT and D-Pad map directly.
- Circle Pad may be quantized to D-Pad by a later task; no analog movement is planned in this stage.
- In debug builds only, `L+R+SELECT` toggles the bottom debug overlay.
- Release builds do not intercept the overlay chord.
- START is a bootstrap exit control until normal game flow owns it.

## Filesystem And Assets

Game-facing paths are relative and never contain `romfs:/` or `sdmc:/`. The 3DS backend maps resource reads to RomFS. The existing generated resource pipeline remains authoritative; `make 3ds` asks Ninja for allowlisted generated assets and stages them in the mode-specific build directory before packaging. Generated NARCs are not copied into the source tree or committed. Original NARC and related formats are retained wherever their parsers are portable. No commercial ROM is downloaded or committed.

The initial filesystem API supports file sizing and exact reads. The portable NARC view validates the container header, BTAF/BTNF/GMIF blocks, allocation table, and every member range over a buffer loaded through `PlatformFile`; it does not depend on libctru paths or packed host structures. The bootstrap loads the 585,336-byte generated icon archive into a capacity-tracked heap with a 1 MiB upper bound. Streaming/open/seek primitives will be introduced before substantially larger archives are connected.

## Graphics

Existing CPU-side game state is progressively separated from DS submission mechanisms:

```text
BG/window/sprite/model state -> renderer interface -> Citro2D/Citro3D
```

VRAM banks, OAM addresses, GX/G2 registers, HBlank writes, and DS texture proxies are not emulated as physical hardware. The renderer reproduces their visible semantics. HBlank effects may be implemented as precomputed scanline/mesh effects.

The native presentation base owns Citro3D and Citro2D initialization, one render target for each physical screen, frame submission, and a bounded GPU text buffer. Bootstrap and fatal diagnostics use the same GPU path instead of mixing software consoles with Citro3D framebuffers. Each screen also owns a row-major `256x192` RGBA logical surface; the backend converts it to PICA tiled texture layout and presents it nearest-filtered at `320x240`. The top image is centered at physical x=40 while the bottom image fills its 320-pixel width. Game render commands remain a separate follow-up layer.

The sprite path supports tiled uploads, repeated image submission, independent position/scale/rotation, source transparency, and per-instance alpha blending. Bounds-checked portable readers cover 4bpp NCGR character, NCLR palette, and text-mode NSCR data. Text BG decoding resolves 10-bit tile indices, per-entry palette banks, horizontal/vertical flips, and a caller-selected viewport into a logical surface. The bootstrap decodes Turtwig's first `32x32` frame and the Underground top-screen `256x192` background directly from generated NARCs; no source PNG is packaged into RomFS.

Portable field-dialogue support decrypts original message-bank entries, reads variable-width 2bpp NFGR glyphs, assembles the 18-role message frame, and interprets ordinary glyphs, authored newlines, color changes, and EOS. Normal speed reveals one glyph every four game ticks; control codes consume no visible delay. The initial transition subset reproduces tick-counted black brightness fades through a GPU overlay. Paging, substitutions, scrolling, other control codes, and shaped fades remain follow-up work.

## Audio

High-level music/SFX/cry policy remains game logic. NNS Sound and ARM7 services are replaced behind an audio backend. NDSP does not consume SDAT directly, so sequence/bank playback requires either a maintained decoder or an automated build-time conversion; manual per-asset conversion is prohibited.

## Time

The platform uses the ARM11 system counter for monotonic nanoseconds. Logical game cadence is scheduled at the original DS rate of `59.8261 Hz` with a rational accumulator, without binding simulation to physical presentation. Frame synchronization remains a separate VBlank operation. Wall-clock acquisition uses libctru `osGetTime`; portable calendar code converts its 1900 epoch to Platinum's 2000 epoch over the original 2000-2099 range and retains Gregorian leap-year, day-of-year, and Sunday-based week-day calculations. Direct NDS timer registers, RTC IPC, and IRQs are not retained.

## Save

Original save structures, checksums, block counters, and recovery semantics are retained when practical. Physical CARD flash access is replaced by a file-backed 3DS backend rooted at `sdmc:/3ds/pokeplatinum`. Writes are staged and synchronized to a temporary file, then replace the destination through a recoverable backup transaction. Startup reads restore the backup if interruption occurred during replacement and ignore an uncommitted temporary file, preserving the previous valid image.

## Memory

The port preserves layouts and alignment where gameplay or serialized data requires them, but does not reproduce the NDS address map. Portable heaps own contiguous arenas and track capacity, allocation count, requested size, and free ranges without libctru dependencies. Child heaps reserve their arena from the low or high end of a parent and return it when destroyed; allocations likewise support aligned low/high placement, arbitrary-order free, coalescing, and reuse. A heap cannot be destroyed while it owns allocations or children. Game heaps and GPU/resource memory are separate. Pointer arithmetic uses `uintptr_t`, not integer casts that assume host pointer size. Direct VRAM, ITCM, DTCM, shared-memory, and register addresses are prohibited in portable code.

## Overlays And ARM7

NDS overlays become statically linked code modules while preserving `ApplicationManager` init/main/exit behavior, including callbacks that span multiple frames. A validated registry maps stable application IDs to linked templates, and a runner owns the current/pending application and procedure state. Queueing changes bookkeeping only; no binary load/unload or dynamic linker is used.

There is no ARM7 emulation. Touch, RTC, power, audio, microphone, save, and communications services are replaced with 3DS services or explicit unsupported behavior.

## Communications

Nintendo WFC and local NDS wireless are outside the first playable port. One synchronous offline backend handles local wireless, Nintendo WFC, GTS, and Mystery Gift probes. Every valid probe returns the terminal `UNAVAILABLE` status and the same understandable message immediately; it never reports a pending operation. This is intentional unsupported behavior rather than a temporary stub. Deferred functions are tracked in `docs/porting/stubs.md`.

## Debug And Errors

The public API provides `Debug_Init`, `Debug_Shutdown`, `Debug_Log`, `Debug_Warn`, `Debug_Error`, `Debug_SetOverlayEnabled`, and `Debug_IsOverlayEnabled`. The 3DS backend retains fixed-size recent log lines, emits external emulator/debugger output, and optionally renders recent lines on the bottom screen.

- Ring buffer: 64 lines, 96 bytes per line.
- Debug overlay toggle: `L+R+SELECT`.
- Debug top screen: current bootstrap/milestone state.
- Debug bottom screen: recent logs and frame/input/resource status.
- Fatal debug errors attempt to show subsystem, function, detail, and controlled START exit.
- Release builds compile out the bottom overlay and its chord; gameplay must never depend on it.

## Acceptance Policy

Every roadmap task includes an observable or automated `Verify:` condition. Runtime work remains `[~]` until exercised in an emulator or hardware. Completed tasks retain a `Result:`. Every executable milestone displays a stable milestone identifier; the initial identifier is `BOOT-01`.
