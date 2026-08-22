# Pokemon Platinum 3DS Port Roadmap

Task states: `[ ]` pending, `[x]` complete, `[-]` blocked, `[~]` in progress.

## Phase 0 - Development Environment

- [x] Record the host distribution and upstream commit.
  - Verify: inspect `SPECS.md` and compare the values with `/etc/os-release` and `git rev-parse HEAD`.
  - Result: CachyOS (Arch family), Linux 7.2.0, upstream commit `6b2e9a5bf94c714ef8ef7cda5be06c0ccee01c70` recorded.
- [x] Validate all host dependencies.
  - Verify: `bison`, `flex`, `g++`, `git`, `make`, `ninja`, `pkg-config`, `python3`, and `arm-none-eabi-gcc` resolve from `PATH`, then `make check` completes configuration.
  - Result: all required commands resolve and the complete NDS configuration/build/test pipeline succeeds.
- [x] Build the original Rev 1 NDS target.
  - Verify: run `make check` and obtain `build/pokeplatinum.us.nds` with SHA-1 `0862ec35b24de5c7e2dcb88c9eea0873110d755c`.
  - Result: `make check` completed 4/4 tests and `sha1sum` matched `0862ec35b24de5c7e2dcb88c9eea0873110d755c`.
- [x] Install and validate devkitPro `3ds-dev`.
  - Verify: `$DEVKITARM/bin/arm-none-eabi-gcc --version` succeeds and libctru is installed.
  - Result: Fish exports `/opt/devkitpro` and `/opt/devkitpro/devkitARM`; devkitARM r68, libctru 2.7.0, Citro2D 1.7.0, Citro3D 1.7.1, and 3dstools 1.3.1 are installed.

## Phase 1 - Native Bootstrap And Debug

- [x] Define the 3DS architecture and build contract.
  - Verify: `SPECS.md` documents target hardware, screens, input, storage, rendering, audio, timing, memory, debug, and unsupported networking.
  - Result: initial living specification created.
- [x] Add an isolated native 3DS target.
  - Verify: inspect the `3ds` entry in `make -f Makefile -qp` and the output contract in `platform/3ds/Makefile`.
  - Result: forwarding target, mode-specific object directories, explicit source allowlist, and isolated outputs added without changing the NDS Meson target.
- [x] Build the first `.3dsx` bootstrap.
  - Verify: run `make 3ds DEBUG=1`, open `build/pokeplatinum-3ds.3dsx`, and observe `BOOT OK` on the top screen.
  - Result: debug ELF, SMDH, and `.3dsx` compile successfully; the `.3dsx` SHA-256 is `c2cb53d4d871db5c6f47161b109283af50e33fb062cc7ec483f92e404c83c4bf`, and Azahar 2126.0 displays `BOOT OK`.
- [x] Provide a backend-independent Debug API and ring buffer.
  - Verify: boot the debug build and observe startup messages retained on the bottom screen after more than one frame.
  - Result: API, fixed-size ring buffer, external debug output, and retained startup messages verified in Azahar.
- [x] Toggle the debug overlay with `L+R+SELECT`.
  - Verify: the chord hides and restores the bottom-screen log without restarting.
  - Result: the isolated automation test hid and restored the bottom log without restarting Azahar.
- [x] Compile a release build without the debug overlay.
  - Verify: run `make 3ds DEBUG=0`; the app boots without a bottom debug console and `L+R+SELECT` is not intercepted.
  - Result: release artifact `6a645b6bff98fc9df85e5cc56fe5de03d576b354587d17486c8c52b736f3d170` booted without a bottom console; the chord remained normal game input and did not reveal an overlay.
- [x] Show controlled fatal errors in debug builds.
  - Verify: invoke `Debug_Fatal` in a smoke build and observe subsystem, function, error, and `Press START to exit`.
  - Result: `make 3ds DEBUG=1 FATAL_SMOKE=1` displayed subsystem `BOOT`, function `main`, the deliberate error, and `Press START to exit`; START then closed the smoke test.
- [x] Add project-local Azahar automation tooling.
  - Verify: `opencode mcp list` reports `azahar connected`, its JSON-RPC smoke test lists seven tools, and all graphical operations stay on a private Xvfb display.
  - Result: launch, status, screenshots, keyboard controls, touch, logs, stop, rebuild, and optional GDB startup are available through `.opencode/mcp/azahar_mcp.py`; non-isolated launch, capture, and input are rejected.

## Phase 2 - Platform Foundations

- [x] Read digital 3DS input through `PlatformInput`.
  - Verify: pressing A/B/X/Y/L/R/START/SELECT/D-Pad updates the top-screen key indicators.
  - Result: A/B/X/Y/L/R/SELECT and every D-Pad direction updated the diagnostics in Azahar; START also drove the documented exit path.
- [x] Read touch input in DS logical coordinates.
  - Verify: touching all bottom-screen corners reports coordinates within `0..255, 0..191`.
  - Result: isolated corner tests reported logical coordinates from `1,2` through `254,184`, within the required DS ranges.
- [x] Mount RomFS and read a packaged smoke resource.
  - Verify: debug overlay reports `ROMFS OK`, file size, and FNV-1a checksum for `port3ds-smoke.txt`.
  - Result: Azahar reported `ROMFS OK`, loaded `port3ds-smoke.txt`, and displayed size 53 with FNV-1a `9cd859d0`.
- [x] Add monotonic time and fixed game-tick scheduling.
  - Verify: overlay reports stable game ticks at the original logical cadence for ten minutes.
  - Result: the ARM11 monotonic counter and `59.8261 Hz` rational scheduler ran for approximately 622.5 seconds in isolated Azahar; the overlay reported 37,243 ticks at 59.825 Hz versus 37,246 presentation frames.
- [x] Add a portable heap backend.
  - Verify: allocate, align, write, read, and free from representative heap IDs; overlay reports `HEAP TEST OK`.
  - Result: isolated Azahar displayed `HEAP TEST OK` after exercising SYSTEM, DEBUG, and APPLICATION heaps with 8-, 32-, and 64-byte alignment, data patterns, ownership metadata, counters, free, and destroy.
- [x] Add file-backed save storage with atomic replacement.
  - Verify: write a test value, restart the `.3dsx`, and recover the same value after a forced interruption test.
  - Result: after staging `eca86420` and forcibly stopping Azahar, restart retained `13579bdf` and reported the staged data ignored; an atomic commit followed by another restart recovered `eca86420`.

## Phase 3 - Asset And Renderer Bootstrap

- [x] Export generated NDS resources into 3DS RomFS without commercial ROM data.
  - Verify: load a generated Platinum NARC and match its expected checksum.
  - Result: the build generated and staged `res/pokemon/evo.narc` without adding it to the source tree; isolated Azahar loaded 26,468 bytes and reported FNV-1a `a955d414` plus `NARC EXPORT OK`.
- [ ] Parse a real NARC through `PlatformFile`.
  - Verify: list member count and sizes for a selected archive in the debug overlay.
- [ ] Initialize Citro2D/Citro3D render targets.
  - Verify: top and bottom screens display distinct test colors and overlay reports `GPU OK`.
- [ ] Present logical 256x192 top and bottom surfaces.
  - Verify: a pixel-grid test scales to 320x240 with the top image centered in 400x240.
- [ ] Render a test texture, four sprites, and alpha blending.
  - Verify: emulator shows the expected texture and four independently positioned sprites, one semitransparent.
- [ ] Decode and render one real Platinum 2D asset.
  - Verify: a recognizable sprite is visible without palette or tile corruption.

## Phase 4 - Runtime Extraction

- [ ] Replace the NDS infinite loop with init/frame/shutdown game boundaries.
  - Verify: a host smoke test executes a fixed number of game frames and exits cleanly.
- [ ] Run the portable task manager on 3DS.
  - Verify: main, frame-boundary, print, and post-frame tasks increment separate visible counters in order.
- [ ] Replace dynamic NDS overlays with statically linked application registration.
  - Verify: switch between two test applications and show their init/main/exit sequence in debug.
- [ ] Port RTC acquisition while preserving calendar calculations.
  - Verify: displayed date/time matches the 3DS system clock and a day rollover test passes.
- [ ] Port game heap hierarchy and allocation direction semantics.
  - Verify: representative game allocations pass alignment and high/low allocation tests.
- [ ] Disable unsupported communications through one explicit offline backend.
  - Verify: networking menu probes return a visible unavailable message without hanging or crashing.

## Phase 5 - UI And Field Demo

- [ ] Render DS BG tilemaps and palettes through the 3DS renderer.
  - Verify: a real Platinum background matches a reference capture.
- [ ] Render windows, fonts, control codes, fades, and transitions.
  - Verify: a real dialogue box displays wrapped text with correct colors and timing.
- [ ] Render OAM-compatible sprites and animations.
  - Verify: four real animated sprites show correct frame order, priority, palette, and alpha.
- [ ] Load and display a real map and its metadata.
  - Verify: debug shows correct map ID/dimensions and the map is recognizable.
- [ ] Draw the player and NPCs with correct depth ordering.
  - Verify: characters pass in front of and behind a known map object correctly.
- [ ] Move the player with collisions and camera tracking.
  - Verify: D-Pad moves tile-by-tile, a known wall blocks movement, and the camera follows.
- [ ] Execute dialogue, choices, flags, item grants, NPC movement, and warps.
  - Verify: one test NPC exercises each operation and a doorway loads another map.

## Phase 6 - Boot, Menus, And Battle

- [ ] Restore logos, title screen, Continue, and New Game.
  - Verify: a fresh boot reaches an interactive title and starts the introduction.
- [ ] Port Start menu, Bag, party, Pokedex, summary, options, shops, and basic PC.
  - Verify: each menu opens, navigates, performs one representative action, and returns to the field.
- [ ] Render a complete wild battle.
  - Verify: encounter, command selection, move damage, faint/run result, and field return all work.
- [ ] Render a complete trainer battle.
  - Verify: trainer intro, switching, victory result, reward, and field return all work.
- [ ] Cover double and special battle cases.
  - Verify: reproducible debug battle fixtures complete without state or presentation errors.

## Phase 7 - Audio And Save

- [ ] Initialize NDSP and play a generated test tone.
  - Verify: tone is audible in emulator and hardware smoke tests.
- [ ] Automate conversion/playback of Platinum SFX, cries, and music.
  - Verify: one SFX, one cry, and one looping map track play with correct transitions.
- [ ] Connect original save serialization to 3DS storage.
  - Verify: save player position, restart, choose Continue, and respawn at that position.
- [ ] Validate checksums, redundancy, and interrupted-write recovery.
  - Verify: corrupt the primary copy and recover from backup; interrupt a write without losing the previous save.

## Phase 8 - Full Game Compatibility

- [ ] Maintain the compatibility matrix in `docs/porting/compatibility.md`.
  - Verify: every tested feature records build ID, fixture/save, status, and result.
- [ ] Complete a normal New Game playthrough through Elite Four and basic postgame.
  - Verify: checkpoint results cover progression, centers, shops, PC, HMs, caves, gyms, evolutions, cutscenes, and credits.
- [ ] Meet stable original-speed performance on Old 3DS.
  - Verify: measured frame pacing remains within the documented budget in field, menus, and representative battles.
- [ ] Produce release documentation and a reproducible release `.3dsx`.
  - Verify: a clean Linux environment follows `BUILDING-3DS.md` and reproduces the published artifact.
- [ ] Stop before 3DS-specific enhancements.
  - Verify: no widescreen viewport, stereoscopic 3D, analog movement, 60 FPS, or New 3DS-only feature is included in this stage.
