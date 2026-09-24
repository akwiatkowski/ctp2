#!/usr/bin/env python3
"""Native button transitions must not starve behind mouse-motion backlog."""

import os
from pathlib import Path
import sys
import tempfile

from ctp2_client import Ctp2Client


binary = Path(sys.argv[1]).resolve()
out = Path(tempfile.mkdtemp(prefix="ui-native-input-", dir=binary.parent))
socket = f"/tmp/ctp2-native-input-{os.getpid()}.sock"
env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy",
           SDL_RENDER_DRIVER="software", CTP2_CAPTURE_FRAMES="1",
           CTP2_SMOKE_SOCKET=socket)
print(f"Native input artifacts: {out}", flush=True)

with Ctp2Client(str(binary), "ui", env=env, socket_path=socket,
                log_path=str(out / "game.log"), timeout=5) as client:
    path = "InitPlayWindow.NewGameButton"
    bounds = client.result("ui_control_bounds", path)
    x = bounds["x"] + bounds["width"] // 2
    y = bounds["y"] + bounds["height"] // 2
    client.expect_ok("ui_native_motion_burst", x, y, 200)
    client.expect_ok("ui_native_pointer", x, y, 0)
    client.expect_ok("ui_native_pointer", x, y, 1)
    client.wait_until(
        lambda: client.result("ui_control_bounds", path)["down"],
        timeout=0.5, interval=0.01, desc="native mouse-down drains after motion burst")
    client.expect_ok("ui_native_pointer", x, y, 0)
    client.wait_until(
        lambda: client.result("ui_control_bounds", "SPNewGameWindow.StartButton").get("visible"),
        timeout=2, interval=0.02, desc="native mouse-up executes New Game")
    client.expect_ok("ui_prepare_game", 42, 4)
    start = client.result("ui_control_bounds", "SPNewGameWindow.StartButton")
    start_x = start["x"] + start["width"] // 2
    start_y = start["y"] + start["height"] // 2
    for down in (0, 1, 0):
        client.expect_ok("ui_pointer", start_x, start_y, down)
    client.wait_game_loaded()

    turn_path = "ControlPanelWindow.ControlPanel.TurnButton"
    turn = client.result("ui_control_bounds", turn_path)
    turn_x = turn["x"] + turn["width"] // 2
    turn_y = turn["y"] + turn["height"] // 2
    client.expect_ok("ui_native_pointer", turn_x, turn_y, 1)
    client.wait_until(
        lambda: client.result("ui_control_bounds", turn_path)["down"],
        timeout=0.5, interval=0.01,
        desc="native input resumes after synthetic game setup")
    client.expect_ok("ui_native_pointer", turn_x, turn_y, 0)

print("PASS native button transitions bypass coalesced motion backlog", flush=True)
