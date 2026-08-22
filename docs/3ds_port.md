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

The next runtime slice is the original NARC reader and Nitro FS interface over
RomFS. The long-term entry path remains `src/main.c:NitroMain`; reaching it
requires adapters for the Nitro OS, input, RTC, sound, graphics, save-device,
and remaining overlay boundaries.
