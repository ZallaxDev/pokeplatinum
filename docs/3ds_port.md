# Nintendo 3DS Port

The native target ports the existing game sources. Platform code may adapt
hardware and operating-system services, but it must not replace game state
machines, field rules, scripts, saves, or application flow.

## Build

Install devkitPro's `3ds-dev` toolchain, then run:

```sh
make 3ds
```

The artifact is `build-3ds-real/pokeplatinum-real-port.3dsx`.

The platform-independent smoke can run without devkitPro:

```sh
env -u DEVKITARM -u DEVKITPRO make check-3ds-host
```

## Current milestone

The target recompiles and executes these game sources with devkitARM:

- `src/sys_task_manager.c`
- `src/map_tile_behavior.c`
- `src/overlay_manager.c`
- `src/narc.c`
- `src/rtc.c`
- `NitroSDK/libraries/rtc/src/convert.c`

The libctru entry point is only a verification shell. It does not implement a
parallel frontend or game runtime. Original game translation units retain the
NDS ABI assumptions that matter to shared structures (`signed char` and
32-bit enums); libctru translation units retain libctru's own enum ABI.

`src/overlay_manager.c` runs its original LOAD, INIT, MAIN, and EXIT state
machine. Native overlays are linked statically, so the adapter tracks their
lifetime instead of loading ARM9 binaries into fixed DS addresses. Heap calls
currently use native allocation until the original FND heap implementation is
ported. Its shared header only has GCC portability fixes for an ineffective
scalar `const` qualifier and a typed overlay sentinel; behavior and layout are
unchanged.

`src/heap.c` now owns heap IDs, parent/child creation, low/high allocation,
allocation headers, counters, and destruction. ARM builds use the original
NitroSystem FND exp-heap, list, heap-common, and allocator sources. The native
adapter only supplies process-lifetime arena memory and single-threaded OS
locking. Host tests use an FND shim because the SDK's pointer arithmetic is
intentionally 32-bit.

The original NARC reader opens generated resources through a minimal Nitro
`FSFile` compatibility surface backed by standard native files. The 3DS build
mounts `titledemo.narc` at its original NitroFS path,
`demo/title/titledemo.narc`, and verifies archive metadata plus whole and
partial member reads. Resource paths remain owned by `src/narc.c`.
The RomFS adapter also carries forward the previous branch's bounded path
joining, rejecting absolute paths, traversal, backslashes, and alternate
roots.

`src/rtc.c` retains the game's cached date/time state, ten-frame polling
interval, time-of-day policy, and elapsed-time behavior. The native transport
fills Nitro `RTCDate` and `RTCTime` from the system clock and completes the SDK
callback synchronously. Date/second conversion is the original portable
NitroSDK implementation.

The next runtime slice is the original heap implementation or another
boot-path subsystem that can be isolated cleanly. The long-term entry path
remains `src/main.c:NitroMain`; reaching it requires adapters for the Nitro OS,
input, sound, graphics, save-device, and remaining overlay boundaries.

## Reuse from `3ds-port`

Useful platform work from the experimental branch is imported selectively,
never by cherry-picking its replacement runtime. Current reuse includes build
and RomFS staging patterns, bounded filesystem paths, heap test scenarios, and
Azahar verification. The Citro2D/Citro3D surface backend, HID conversion,
monotonic tick conversion, transactional save writes, and bounded Nitro 2D/3D
resource decoders remain candidates to place beneath original Nitro/NNS/CARD
interfaces. Replacement frontend, task, application, field, script, movement,
checkpoint, NARC, heap, and RTC implementations are not reusable runtime code.
