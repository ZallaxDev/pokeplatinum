# 3DS Compatibility Matrix

| Feature | Status | Build | Verification | Notes |
|---|---|---|---|---|
| Native boot | Pass | BOOT-01 debug `c2cb53d...c83c4bf` | Azahar 2126.0 displays `BOOT OK` | Isolated Xvfb smoke test |
| Debug overlay | Pass | BOOT-01 debug `c2cb53d...c83c4bf` | Startup logs persist; L+R+SELECT hides and restores them | debug build only |
| Fatal screen | Pass | BOOT-01 fatal smoke | Displays subsystem, function, error, and exit prompt; START exits | `FATAL_SMOKE=1` |
| Release diagnostics | Pass | BOOT-01 release `6a645b6b...3d170` | Boots without bottom console; debug chord does not reveal one | release build only |
| Digital input | Pass | BOOT-01 debug `c2cb53d...c83c4bf` | A/B/X/Y/L/R/SELECT and D-Pad observed; START exit path works | bootstrap diagnostics only |
| Touch input | Pass | BOOT-01 debug `c2cb53d...c83c4bf` | Corner smoke tests report `1,2` through `254,184` | DS logical range is `0..255,0..191` |
| RomFS | Pass | BOOT-01 debug `c2cb53d...c83c4bf` | `ROMFS OK`; size 53; FNV-1a `9cd859d0` | `port3ds-smoke.txt` only |
| Monotonic game ticks | Pass | BOOT-01 debug `7e4a3cf...aed5c6` | 37,243 ticks at 59.825 Hz after about 622.5 seconds | Target 59.8261 Hz; 37,246 presentation frames |
| Portable heap base | Pass | BOOT-01 debug `ecf0ea22...08e8a3` | Overlay reports `HEAP TEST OK` for three IDs and alignments | Game hierarchy/direction not connected yet |
| Transactional save base | Pass | BOOT-01 debug `086ac4d8...8c3b9f6` | Interrupted stage retained old value; committed value survived restart | Smoke record only; game serializer not connected |
| Generated NARC export | Pass | BOOT-01 debug `46ec493e...37c53b` | `evo.narc`: 26,468 bytes, FNV-1a `a955d414` | Source artifact SHA-256 `fe6e4ec4...7862d` |
| Portable NARC parser | Pass | BOOT-01 debug `b9889e5c...72adb3` | 508 members; sampled sizes `44/44/44`; `NARC PARSE OK` | Validates all member ranges |
| Citro2D/Citro3D targets | Pass | BOOT-01 debug `6f44ded2...198b8e` | Distinct navy/plum targets at 60 FPS; overlay reports `GPU OK` | Debug, release, and fatal diagnostics use GPU text |
| Logical screen surfaces | Pass | BOOT-01 debug `33ac2372...006adb` | `256x192` pixel grids scale nearest to `320x240`; top centered at x=40 | Overlay reports `SURFACE TEST OK` |
| Texture and sprite submission | Pass | BOOT-01 debug `7cb13497...b4074f` | Four procedural RGBA sprites render with independent transforms; fourth blends at 45% alpha | Overlay reports `SPRITE TEST OK` |
| Platinum 2D asset decode | Pass | BOOT-01 debug `288870ce...64408c` | Turtwig NCGR member 394 renders `32x32` with shared NCLR palette bank 1 and transparency | Overlay reports `PLATINUM ASSET OK` |
| Game lifecycle boundary | Pass | BOOT-01 debug `e752fe7e...36034d` | Host smoke: init=1, frames=120, shutdown=1; Azahar game frames track ticks | Gameplay hook bodies not connected yet |
| Portable task manager | Pass | BOOT-01 debug `b2c73f62...cd2a7b` | Main/boundary/print/after counters remain equal and ordered; host mutation smoke passes | Fixed capacity; game tasks not connected yet |
| Static application registry | Pass | BOOT-01 debug `283692d4...2257a6` | ALPHA/BETA alternate with visible init/main/exit sequence; host sequence smoke passes | Real application templates not registered yet |
| RTC and calendar | Pass | BOOT-01 debug `8f98d553...c6a8ab` | Azahar clock matched host within 4 seconds; host rollover and sanitizer smokes pass | Supports Platinum's 2000-2099 RTC range |
| Gameplay | Not started | - | - | NDS runtime not linked yet |
