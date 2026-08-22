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
| Gameplay | Not started | - | - | NDS runtime not linked yet |
