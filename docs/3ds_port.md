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

The target recompiles and executes these unmodified game sources with
devkitARM:

- `src/sys_task_manager.c`
- `src/map_tile_behavior.c`

The libctru entry point is only a verification shell. It does not implement a
parallel frontend or game runtime. Original game translation units retain the
NDS ABI assumptions that matter to shared structures (`signed char` and
32-bit enums); libctru translation units retain libctru's own enum ABI.

The next runtime slice is the original `src/overlay_manager.c`, backed only by
heap allocation and static native-overlay adapters. The long-term entry path
remains `src/main.c:NitroMain`; reaching it requires adapters for the Nitro OS,
filesystem, input, RTC, sound, graphics, save-device, and overlay boundaries.
