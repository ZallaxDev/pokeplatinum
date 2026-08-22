#!/usr/bin/env python3
"""Small stdio MCP server for exercising the project's 3DS build in Azahar."""

from __future__ import annotations

import base64
import ctypes
import json
import os
from pathlib import Path
import signal
import shutil
import subprocess
import sys
import time
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ARTIFACT = ROOT / "build" / "pokeplatinum-3ds.3dsx"
STATE_DIR = Path("/tmp/opencode/azahar-mcp")
LAUNCH_LOG = STATE_DIR / "launcher.log"
AZAHAR_LOG = (
    Path.home()
    / ".var/app/org.azahar_emu.Azahar/data/azahar-emu/log/azahar_log.txt"
)
APP_ID = "org.azahar_emu.Azahar"

KEYS = {
    "A": "a",
    "B": "s",
    "X": "z",
    "Y": "x",
    "L": "q",
    "R": "w",
    "START": "m",
    "SELECT": "n",
    "UP": "t",
    "DOWN": "g",
    "LEFT": "f",
    "RIGHT": "h",
}

_process: subprocess.Popen[bytes] | None = None
_launch_handle: Any = None
_xvfb: subprocess.Popen[bytes] | None = None
_display_name: str | None = None


TOOLS = [
    {
        "name": "azahar_status",
        "description": "Check Azahar, Flatpak, build artifact, graphical automation, and log availability.",
        "inputSchema": {"type": "object", "properties": {}, "additionalProperties": False},
    },
    {
        "name": "azahar_launch",
        "description": "Optionally rebuild and launch a project .3dsx in Azahar on an isolated Xvfb display.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "artifact": {"type": "string", "description": "Project-relative .3dsx path."},
                "rebuild": {"type": "boolean", "default": False},
                "debug": {"type": "boolean", "default": True},
                "gdb_port": {"type": "integer", "minimum": 1024, "maximum": 65535},
                "headless": {"type": "boolean", "const": True, "default": True},
                "wait_seconds": {"type": "number", "minimum": 0, "maximum": 30, "default": 5},
            },
            "additionalProperties": False,
        },
    },
    {
        "name": "azahar_screenshot",
        "description": "Capture the isolated Azahar display and return the PNG for visual inspection.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "mode": {"type": "string", "enum": ["active", "fullscreen"], "default": "active"},
                "delay_ms": {"type": "integer", "minimum": 0, "maximum": 10000, "default": 500},
            },
            "additionalProperties": False,
        },
    },
    {
        "name": "azahar_key",
        "description": "Send one or more 3DS controls to Azahar. Use names such as A, UP, or L+R+SELECT.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "keys": {"type": "string", "description": "Plus-separated controls."},
                "hold_ms": {"type": "integer", "minimum": 20, "maximum": 5000, "default": 100},
            },
            "required": ["keys"],
            "additionalProperties": False,
        },
    },
    {
        "name": "azahar_click",
        "description": "Click inside the isolated Azahar window for touch tests.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "x": {"type": "number", "minimum": 0, "maximum": 1},
                "y": {"type": "number", "minimum": 0, "maximum": 1},
            },
            "required": ["x", "y"],
            "additionalProperties": False,
        },
    },
    {
        "name": "azahar_logs",
        "description": "Read recent Azahar native and MCP launcher logs.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "lines": {"type": "integer", "minimum": 1, "maximum": 1000, "default": 200}
            },
            "additionalProperties": False,
        },
    },
    {
        "name": "azahar_stop",
        "description": "Stop the Azahar Flatpak instance launched for testing.",
        "inputSchema": {"type": "object", "properties": {}, "additionalProperties": False},
    },
]


def text_result(text: str, error: bool = False) -> dict[str, Any]:
    result: dict[str, Any] = {"content": [{"type": "text", "text": text}]}
    if error:
        result["isError"] = True
    return result


def run(command: list[str], timeout: float = 30) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, capture_output=True, text=True, timeout=timeout, check=False)


def project_path(value: str | None) -> Path:
    path = DEFAULT_ARTIFACT if value is None else (ROOT / value)
    path = path.resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError("artifact must be inside the project")
    if path.suffix.lower() != ".3dsx":
        raise ValueError("artifact must have a .3dsx extension")
    return path


def flatpak_running() -> bool:
    result = run(["flatpak", "ps", "--columns=application"], timeout=10)
    return APP_ID in result.stdout.splitlines()


def stop_azahar() -> str:
    global _process, _launch_handle, _xvfb, _display_name
    messages = []
    if _process is not None and _process.poll() is None:
        os.killpg(_process.pid, signal.SIGTERM)
        try:
            _process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(_process.pid, signal.SIGKILL)
        messages.append("Azahar stopped")
        run(["flatpak", "kill", APP_ID], timeout=15)
    else:
        messages.append("MCP Azahar instance was not running")
    _process = None
    if _launch_handle is not None:
        _launch_handle.close()
        _launch_handle = None
    if _xvfb is not None and _xvfb.poll() is None:
        _xvfb.terminate()
        try:
            _xvfb.wait(timeout=3)
        except subprocess.TimeoutExpired:
            _xvfb.kill()
        messages.append("virtual display stopped")
    _xvfb = None
    _display_name = None
    return "; ".join(filter(None, messages))


class X11Controller:
    def __init__(self, display_name: str | None = None) -> None:
        self.x11 = ctypes.CDLL("libX11.so.6")
        self.xtst = ctypes.CDLL("libXtst.so.6")
        self.x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
        self.x11.XOpenDisplay.restype = ctypes.c_void_p
        display_value = display_name.encode() if display_name else None
        self.display = self.x11.XOpenDisplay(display_value)
        if not self.display:
            raise RuntimeError("cannot open the isolated X display")
        self.x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
        self._error_callback_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)
        self._error_callback = self._error_callback_type(lambda _display, _event: 0)
        self.x11.XSetErrorHandler.argtypes = [self._error_callback_type]
        self.x11.XSetErrorHandler.restype = ctypes.c_void_p
        self._previous_error_handler = self.x11.XSetErrorHandler(self._error_callback)
        self.x11.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
        self.x11.XDefaultRootWindow.restype = ctypes.c_ulong
        self.x11.XQueryTree.argtypes = [
            ctypes.c_void_p,
            ctypes.c_ulong,
            ctypes.POINTER(ctypes.c_ulong),
            ctypes.POINTER(ctypes.c_ulong),
            ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)),
            ctypes.POINTER(ctypes.c_uint),
        ]
        self.x11.XFetchName.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_char_p)]
        self.x11.XGetWindowProperty.argtypes = [
            ctypes.c_void_p,
            ctypes.c_ulong,
            ctypes.c_ulong,
            ctypes.c_long,
            ctypes.c_long,
            ctypes.c_int,
            ctypes.c_ulong,
            ctypes.POINTER(ctypes.c_ulong),
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_ulong),
            ctypes.POINTER(ctypes.c_ulong),
            ctypes.POINTER(ctypes.POINTER(ctypes.c_ubyte)),
        ]
        self.x11.XFree.argtypes = [ctypes.c_void_p]
        self.x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
        self.x11.XStringToKeysym.restype = ctypes.c_ulong
        self.x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        self.x11.XKeysymToKeycode.restype = ctypes.c_uint
        self.x11.XSetInputFocus.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
        self.x11.XRaiseWindow.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        self.x11.XFlush.argtypes = [ctypes.c_void_p]
        self.x11.XSync.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.x11.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        self.x11.XInternAtom.restype = ctypes.c_ulong
        self.x11.XSendEvent.argtypes = [
            ctypes.c_void_p,
            ctypes.c_ulong,
            ctypes.c_int,
            ctypes.c_long,
            ctypes.c_void_p,
        ]
        self.x11.XGetGeometry.argtypes = [
            ctypes.c_void_p,
            ctypes.c_ulong,
            ctypes.POINTER(ctypes.c_ulong),
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_uint),
            ctypes.POINTER(ctypes.c_uint),
            ctypes.POINTER(ctypes.c_uint),
            ctypes.POINTER(ctypes.c_uint),
        ]
        self.x11.XTranslateCoordinates.argtypes = [
            ctypes.c_void_p,
            ctypes.c_ulong,
            ctypes.c_ulong,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_ulong),
        ]
        self.xtst.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
        self.xtst.XTestFakeMotionEvent.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_ulong]
        self.xtst.XTestFakeButtonEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]

    def close(self) -> None:
        self.x11.XSync(self.display, False)
        if self._previous_error_handler:
            self.x11.XSetErrorHandler(
                ctypes.cast(self._previous_error_handler, self._error_callback_type)
            )
        self.x11.XCloseDisplay(self.display)

    def _name(self, window: int) -> str:
        value = ctypes.c_char_p()
        if self.x11.XFetchName(self.display, window, ctypes.byref(value)) and value.value:
            name = value.value.decode(errors="replace")
            self.x11.XFree(value)
            return name
        property_atom = self.x11.XInternAtom(self.display, b"_NET_WM_NAME", False)
        actual_type = ctypes.c_ulong()
        actual_format = ctypes.c_int()
        item_count = ctypes.c_ulong()
        bytes_after = ctypes.c_ulong()
        data = ctypes.POINTER(ctypes.c_ubyte)()
        result = self.x11.XGetWindowProperty(
            self.display,
            window,
            property_atom,
            0,
            1024,
            False,
            0,
            ctypes.byref(actual_type),
            ctypes.byref(actual_format),
            ctypes.byref(item_count),
            ctypes.byref(bytes_after),
            ctypes.byref(data),
        )
        if result == 0 and data and item_count.value:
            try:
                return ctypes.string_at(data, item_count.value).decode(errors="replace")
            finally:
                self.x11.XFree(data)
        return ""

    def _contains_named_window(self, window: int) -> bool:
        if "azahar" in self._name(window).lower():
            return True
        root = ctypes.c_ulong()
        parent = ctypes.c_ulong()
        children = ctypes.POINTER(ctypes.c_ulong)()
        count = ctypes.c_uint()
        if not self.x11.XQueryTree(
            self.display,
            window,
            ctypes.byref(root),
            ctypes.byref(parent),
            ctypes.byref(children),
            ctypes.byref(count),
        ):
            return False
        try:
            for index in range(count.value - 1, -1, -1):
                if self._contains_named_window(children[index]):
                    return True
        finally:
            if children:
                self.x11.XFree(children)
        return False

    def window(self) -> int:
        root_window = self.x11.XDefaultRootWindow(self.display)
        root = ctypes.c_ulong()
        parent = ctypes.c_ulong()
        children = ctypes.POINTER(ctypes.c_ulong)()
        count = ctypes.c_uint()
        if not self.x11.XQueryTree(
            self.display,
            root_window,
            ctypes.byref(root),
            ctypes.byref(parent),
            ctypes.byref(children),
            ctypes.byref(count),
        ):
            raise RuntimeError("cannot inspect X11 windows")
        candidates: list[tuple[int, int]] = []
        try:
            for index in range(count.value - 1, -1, -1):
                top_level = children[index]
                if self._contains_named_window(top_level):
                    geometry = self._geometry(top_level)
                    candidates.append((geometry[2] * geometry[3], top_level))
        finally:
            if children:
                self.x11.XFree(children)
        if candidates:
            return max(candidates)[1]
        raise RuntimeError("Azahar X11 window not found")

    def _geometry(self, window: int) -> tuple[int, int, int, int]:
        root = ctypes.c_ulong()
        x = ctypes.c_int()
        y = ctypes.c_int()
        width = ctypes.c_uint()
        height = ctypes.c_uint()
        border = ctypes.c_uint()
        depth = ctypes.c_uint()
        if not self.x11.XGetGeometry(
            self.display,
            window,
            ctypes.byref(root),
            ctypes.byref(x),
            ctypes.byref(y),
            ctypes.byref(width),
            ctypes.byref(height),
            ctypes.byref(border),
            ctypes.byref(depth),
        ):
            raise RuntimeError("cannot read Azahar window geometry")
        return x.value, y.value, width.value, height.value

    def focus(self) -> int:
        window = self.window()
        if _display_name:
            self.x11.XRaiseWindow(self.display, window)
            self.x11.XSetInputFocus(self.display, window, 1, 0)
            self.x11.XFlush(self.display)
            time.sleep(0.1)
            return window
        root = self.x11.XDefaultRootWindow(self.display)
        active_window = self.x11.XInternAtom(self.display, b"_NET_ACTIVE_WINDOW", False)

        class ClientData(ctypes.Union):
            _fields_ = [("longs", ctypes.c_long * 5)]

        class ClientMessage(ctypes.Structure):
            _fields_ = [
                ("type", ctypes.c_int),
                ("serial", ctypes.c_ulong),
                ("send_event", ctypes.c_int),
                ("display", ctypes.c_void_p),
                ("window", ctypes.c_ulong),
                ("message_type", ctypes.c_ulong),
                ("format", ctypes.c_int),
                ("data", ClientData),
            ]

        event = ClientMessage()
        event.type = 33
        event.display = self.display
        event.window = window
        event.message_type = active_window
        event.format = 32
        event.data.longs[0] = 2
        event.data.longs[1] = 0
        mask = (1 << 20) | (1 << 19)
        self.x11.XSendEvent(self.display, root, False, mask, ctypes.byref(event))
        self.x11.XFlush(self.display)
        time.sleep(0.2)
        return window

    def keys(self, names: list[str], hold_ms: int) -> None:
        self.focus()
        codes = []
        for name in names:
            keysym = self.x11.XStringToKeysym(KEYS[name].encode())
            code = self.x11.XKeysymToKeycode(self.display, keysym)
            if not code:
                raise RuntimeError(f"X11 keycode unavailable for {name}")
            codes.append(code)
            self.xtst.XTestFakeKeyEvent(self.display, code, True, 0)
        self.x11.XFlush(self.display)
        time.sleep(hold_ms / 1000)
        for code in reversed(codes):
            self.xtst.XTestFakeKeyEvent(self.display, code, False, 0)
        self.x11.XFlush(self.display)

    def click(self, x_ratio: float, y_ratio: float) -> tuple[int, int]:
        window = self.focus()
        _, _, width, height = self._geometry(window)
        root = self.x11.XDefaultRootWindow(self.display)
        root_x = ctypes.c_int()
        root_y = ctypes.c_int()
        child = ctypes.c_ulong()
        self.x11.XTranslateCoordinates(
            self.display,
            window,
            root,
            0,
            0,
            ctypes.byref(root_x),
            ctypes.byref(root_y),
            ctypes.byref(child),
        )
        target_x = root_x.value + round((width - 1) * x_ratio)
        target_y = root_y.value + round((height - 1) * y_ratio)
        self.xtst.XTestFakeMotionEvent(self.display, -1, target_x, target_y, 0)
        self.xtst.XTestFakeButtonEvent(self.display, 1, True, 0)
        self.x11.XFlush(self.display)
        time.sleep(0.15)
        self.xtst.XTestFakeButtonEvent(self.display, 1, False, 0)
        self.x11.XFlush(self.display)
        return target_x, target_y


def with_x11(action: Any, display_name: str | None = None) -> Any:
    controller = X11Controller(display_name)
    try:
        return action(controller)
    finally:
        controller.close()


def read_tail(path: Path, lines: int) -> str:
    if not path.is_file():
        return f"{path}: unavailable"
    data = path.read_text(errors="replace").splitlines()
    return "\n".join(data[-lines:]) or "(empty)"


def status() -> dict[str, Any]:
    flatpak = run(["flatpak", "info", APP_ID], timeout=15)
    details = {
        "project": str(ROOT),
        "artifact": str(DEFAULT_ARTIFACT),
        "artifact_exists": DEFAULT_ARTIFACT.is_file(),
        "artifact_size": DEFAULT_ARTIFACT.stat().st_size if DEFAULT_ARTIFACT.is_file() else 0,
        "flatpak_installed": flatpak.returncode == 0,
        "flatpak_running": flatpak_running(),
        "xvfb": shutil.which("Xvfb") is not None,
        "x11_automation": shutil.which("Xvfb") is not None,
        "image_capture": shutil.which("magick") is not None,
        "execution_mode": "isolated Xvfb" if _display_name else "not running",
        "pointer_automation": bool(_display_name),
        "native_log": str(AZAHAR_LOG),
    }
    return text_result(json.dumps(details, indent=2))


def launch(arguments: dict[str, Any]) -> dict[str, Any]:
    global _process, _launch_handle, _xvfb, _display_name
    artifact = project_path(arguments.get("artifact"))
    debug = bool(arguments.get("debug", True))
    if arguments.get("headless", True) is not True:
        return text_result("Only isolated Xvfb execution is allowed.", True)
    if arguments.get("rebuild", False):
        environment = os.environ.copy()
        environment.update(
            {
                "DEVKITPRO": "/opt/devkitpro",
                "DEVKITARM": "/opt/devkitpro/devkitARM",
                "PATH": f"/opt/devkitpro/devkitARM/bin:/opt/devkitpro/tools/bin:{environment.get('PATH', '')}",
            }
        )
        result = subprocess.run(
            ["make", "3ds", f"DEBUG={1 if debug else 0}"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=180,
            check=False,
            env=environment,
        )
        if result.returncode != 0:
            return text_result(f"3DS rebuild failed:\n{result.stdout}\n{result.stderr}", True)
    if not artifact.is_file():
        return text_result(f"Artifact does not exist: {artifact}", True)

    stop_azahar()
    if flatpak_running():
        return text_result(
            "Another Azahar instance is already running. Close it manually before starting the isolated test.",
            True,
        )
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    _launch_handle = LAUNCH_LOG.open("wb")
    for number in range(90, 120):
        if not Path(f"/tmp/.X11-unix/X{number}").exists():
            _display_name = f":{number}"
            break
    if _display_name is None:
        return text_result("No free Xvfb display is available", True)
    _xvfb = subprocess.Popen(
        [
            "Xvfb",
            _display_name,
            "-screen",
            "0",
            "1280x960x24",
            "-ac",
            "+extension",
            "GLX",
        ],
        stdout=_launch_handle,
        stderr=subprocess.STDOUT,
    )
    time.sleep(1)
    if _xvfb.poll() is not None:
        return text_result(f"Xvfb failed:\n{read_tail(LAUNCH_LOG, 100)}", True)
    command = [
        "flatpak",
        "run",
        "--nosocket=x11",
        "--filesystem=/tmp/.X11-unix",
        "--env=QT_QPA_PLATFORM=xcb",
        "--command=sh",
        APP_ID,
        "-c",
        'export DISPLAY="$1"; shift; exec azahar-launcher "$@"',
        "azahar-mcp",
        _display_name,
        "-w",
    ]
    if "gdb_port" in arguments:
        command.extend(["-g", str(arguments["gdb_port"])])
    command.append(str(artifact))
    _process = subprocess.Popen(
        command,
        cwd=ROOT,
        stdout=_launch_handle,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    wait_seconds = float(arguments.get("wait_seconds", 5))
    time.sleep(wait_seconds)
    if _process.poll() is not None and not flatpak_running():
        return text_result(f"Azahar exited during launch:\n{read_tail(LAUNCH_LOG, 100)}", True)
    return text_result(
        f"Azahar launched with {artifact.name}; pid={_process.pid}; "
        f"display={_display_name or 'desktop'}; gdb_port={arguments.get('gdb_port', 'disabled')}"
    )


def screenshot(arguments: dict[str, Any]) -> dict[str, Any]:
    mode = arguments.get("mode", "active")
    delay_ms = int(arguments.get("delay_ms", 500))
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    output = STATE_DIR / f"screenshot-{time.time_ns()}.png"
    if not _display_name:
        return text_result("Screenshot capture requires an isolated Azahar instance.", True)
    command = [
        "magick",
        "import",
        "-display",
        _display_name,
        "-window",
        "root",
        f"png:{output}",
    ]
    if delay_ms:
        time.sleep(delay_ms / 1000)
    result = run(command, timeout=max(15, delay_ms / 1000 + 10))
    for _ in range(20):
        if output.is_file() and output.stat().st_size:
            break
        time.sleep(0.1)
    if not output.is_file() or not output.stat().st_size:
        return text_result(f"Screenshot failed: {result.stderr or result.stdout}", True)
    encoded = base64.b64encode(output.read_bytes()).decode()
    return {
        "content": [
            {"type": "text", "text": f"Captured {mode} screenshot: {output}"},
            {"type": "image", "data": encoded, "mimeType": "image/png"},
        ]
    }


def send_keys(arguments: dict[str, Any]) -> dict[str, Any]:
    names = [part.strip().upper() for part in arguments["keys"].split("+") if part.strip()]
    if not names or any(name not in KEYS for name in names):
        return text_result(f"Unknown controls. Supported: {', '.join(KEYS)}", True)
    hold_ms = int(arguments.get("hold_ms", 100))
    if not _display_name:
        return text_result("Keyboard automation requires an isolated headless Azahar instance.", True)
    with_x11(lambda controller: controller.keys(names, hold_ms), _display_name)
    return text_result(f"Sent {'+'.join(names)} for {hold_ms} ms")


def click(arguments: dict[str, Any]) -> dict[str, Any]:
    if not _display_name:
        return text_result(
            "Pointer automation requires an isolated headless Azahar instance.",
            True,
        )
    coordinates = with_x11(
        lambda controller: controller.click(float(arguments["x"]), float(arguments["y"])),
        _display_name,
    )
    return text_result(f"Clicked Azahar window at virtual coordinates {coordinates[0]},{coordinates[1]}")


def call_tool(name: str, arguments: dict[str, Any]) -> dict[str, Any]:
    try:
        if name == "azahar_status":
            return status()
        if name == "azahar_launch":
            return launch(arguments)
        if name == "azahar_screenshot":
            return screenshot(arguments)
        if name == "azahar_key":
            return send_keys(arguments)
        if name == "azahar_click":
            return click(arguments)
        if name == "azahar_logs":
            lines = int(arguments.get("lines", 200))
            return text_result(
                f"== Azahar native log ==\n{read_tail(AZAHAR_LOG, lines)}\n\n"
                f"== MCP launcher log ==\n{read_tail(LAUNCH_LOG, lines)}"
            )
        if name == "azahar_stop":
            return text_result(stop_azahar())
        return text_result(f"Unknown tool: {name}", True)
    except Exception as error:
        return text_result(f"{type(error).__name__}: {error}", True)


def response(identifier: Any, result: Any = None, error: Any = None) -> None:
    payload: dict[str, Any] = {"jsonrpc": "2.0", "id": identifier}
    if error is not None:
        payload["error"] = error
    else:
        payload["result"] = result
    sys.stdout.write(json.dumps(payload, separators=(",", ":")) + "\n")
    sys.stdout.flush()


def handle(message: dict[str, Any]) -> None:
    identifier = message.get("id")
    method = message.get("method")
    if identifier is None:
        return
    if method == "initialize":
        requested = message.get("params", {}).get("protocolVersion", "2024-11-05")
        response(
            identifier,
            {
                "protocolVersion": requested,
                "capabilities": {"tools": {"listChanged": False}},
                "serverInfo": {"name": "pokeplatinum-azahar", "version": "1.0.0"},
            },
        )
    elif method == "ping":
        response(identifier, {})
    elif method == "tools/list":
        response(identifier, {"tools": TOOLS})
    elif method == "tools/call":
        params = message.get("params", {})
        response(identifier, call_tool(params.get("name", ""), params.get("arguments", {})))
    else:
        response(identifier, error={"code": -32601, "message": f"Method not found: {method}"})


def main() -> int:
    STATE_DIR.mkdir(parents=True, exist_ok=True)
    for line in sys.stdin:
        try:
            handle(json.loads(line))
        except Exception as error:
            print(f"azahar-mcp: {type(error).__name__}: {error}", file=sys.stderr, flush=True)
    if _process is not None and _process.poll() is None:
        stop_azahar()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
