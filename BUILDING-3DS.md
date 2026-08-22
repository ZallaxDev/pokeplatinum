# Building The Native 3DS Bootstrap

The 3DS target is independent of the existing NDS Meson target. It currently builds milestone `BOOT-01`, not the game runtime.

## Requirements

Install the current devkitPro package manager for your Linux distribution by following <https://devkitpro.org/wiki/Getting_Started>, then install the `3ds-dev` package group:

```bash
sudo dkp-pacman -Syu
sudo dkp-pacman -S 3ds-dev
```

Use `pacman` instead of `dkp-pacman` only when that is how the official devkitPro repository is configured. Export the locations selected by the installer, commonly:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM="$DEVKITPRO/devkitARM"
```

For Fish, configure universal exported variables instead:

```fish
set -Ux DEVKITPRO /opt/devkitpro
set -Ux DEVKITARM $DEVKITPRO/devkitARM
fish_add_path -U $DEVKITARM/bin $DEVKITPRO/tools/bin
```

Validate the supported compiler rather than a system-only ARM compiler:

```bash
"$DEVKITARM/bin/arm-none-eabi-gcc" --version
pacman -Qg 3ds-dev
```

## Build

Debug build with the bottom-screen GPU ring-buffer overlay:

```bash
make 3ds DEBUG=1
```

The NDS build directory must remain configured. The 3DS target invokes Ninja for its allowlisted generated resources, then stages those files in the ignored mode-specific build directory used as RomFS.

Run the portable lifecycle smoke test independently with:

```bash
make check-3ds-host
```

The host checks initialize the game boundary once, execute exactly 120 logical frames, shut down once, and reject frames after shutdown. They also verify task priority and mutation safety, plus the complete init/main/exit sequence while switching between two statically registered applications. All tests run automatically during `make 3ds`.

To build the deliberate fatal-screen smoke test:

```sh
make 3ds DEBUG=1 FATAL_SMOKE=1
```

Release-style bootstrap without the bottom debug overlay or its input chord:

```bash
make 3ds DEBUG=0
```

Outputs:

```text
build/pokeplatinum-3ds.elf
build/pokeplatinum-3ds.3dsx
```

Debug and release objects are isolated in `build-3ds-debug/` and `build-3ds-release/`, so switching `DEBUG` modes cannot reuse incompatible objects. Clean all 3DS artifacts with `make clean-3ds`. The existing `make check` remains the NDS baseline build.

## BOOT-01 Verification

1. Open `build/pokeplatinum-3ds.3dsx` in a compatible emulator or Homebrew Launcher.
2. Confirm the top screen reports `BOOT OK` and milestone `BOOT-01`.
3. Confirm the bottom debug screen reports `Debug_Init OK`, `ROMFS OK`, and the smoke resource size/checksum.
4. Press each face button and D-Pad direction and confirm the top and bottom diagnostics change.
5. Touch the bottom screen and confirm logical coordinates stay inside 256x192.
6. Press `L+R+SELECT` in the debug build and confirm the overlay hides and returns.
7. Press START and confirm controlled exit.
