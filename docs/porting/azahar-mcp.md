# Azahar MCP

The project-local `azahar` MCP drives the Flatpak build of Azahar for observable 3DS smoke tests. It is configured in `.opencode/opencode.json` and implemented without third-party Python packages.

## Tools

- `azahar_status`: check Flatpak, artifact, Xvfb, image capture, and log availability.
- `azahar_launch`: optionally rebuild and launch a project `.3dsx`; an optional GDB stub port can be enabled.
- `azahar_screenshot`: return a PNG of the private Xvfb display to the model.
- `azahar_key`: send 3DS controls using Azahar's current default keyboard profile.
- `azahar_click`: click normalized coordinates inside the isolated Azahar window for touch tests.
- `azahar_logs`: return recent native and launcher logs.
- `azahar_stop`: stop the test instance.

Azahar is always launched in a private Xvfb display with `QT_QPA_PLATFORM=xcb`. Screenshots, keyboard events, and pointer events remain inside that virtual display and cannot capture or control the user's desktop session. Non-isolated launch and capture are rejected. Keyboard automation uses XTest and is restricted to the predefined 3DS controls. Artifact paths are restricted to this repository.

The default control mapping read from Azahar 2126.0 is:

| 3DS | Keyboard |
|---|---|
| A/B/X/Y | A/S/Z/X |
| L/R | Q/W |
| START/SELECT | M/N |
| Up/Down/Left/Right | T/G/F/H |

Screenshots and launcher output are temporary files under `/tmp/opencode/azahar-mcp`. Azahar's native log remains under its Flatpak data directory. If another manually launched Azahar instance is running, the MCP refuses to start instead of closing or reusing it.

## Activation

OpenCode loads MCP configuration only at startup. Quit and restart OpenCode from this repository after changing `.opencode/opencode.json`. The first call should be `azahar_status`, followed by `azahar_launch` and `azahar_screenshot`.
