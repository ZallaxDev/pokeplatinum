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
| Gameplay | Not started | - | - | NDS runtime not linked yet |
