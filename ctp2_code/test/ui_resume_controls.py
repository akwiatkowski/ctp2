#!/usr/bin/env python3
"""Recorded session 20260923T170854Z: resumed UI controls appeared dead.

The restored Director carried one orphan pending action, so Next Turn requests
never executed. Queue native SDL down/up events, prove button-down reaches the
real TurnButton, then require the normal pipeline to advance round 1 to 2.
"""

import os
from pathlib import Path
import sys
import tempfile

from ctp2_client import Ctp2Client, fixture_save


binary = Path(sys.argv[1]).resolve()
out = Path(tempfile.mkdtemp(prefix="ui-resume-controls-", dir=binary.parent))
socket = f"/tmp/ctp2-resume-controls-{os.getpid()}.sock"
env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy",
           SDL_RENDER_DRIVER="software", CTP2_CAPTURE_FRAMES="1",
           CTP2_SMOKE_SOCKET=socket)
print(f"Resume control artifacts: {out}", flush=True)

with Ctp2Client(str(binary), "ui", env=env, socket_path=socket,
                log_path=str(out / "game.log"), timeout=10) as client:
    client.expect_ok("load_game", fixture_save("next-unit-freeze"))
    client.wait_game_loaded()

    def click(path):
        bounds = client.result("ui_control_bounds", path)
        assert bounds["visible"] and bounds["enabled"], (path, bounds)
        x = bounds["x"] + bounds["width"] // 2
        y = bounds["y"] + bounds["height"] // 2
        client.expect_ok("ui_native_pointer", x, y, 0)
        client.expect_ok("ui_native_pointer", x, y, 1)
        client.wait_until(
            lambda: client.result("ui_control_bounds", path)["down"],
            timeout=5, interval=0.05, desc=f"native button-down reaches {path}")
        client.expect_ok("ui_native_pointer", x, y, 0)

    before = client.result("query_turn")
    print(f"Resumed turn state: {before}", flush=True)
    frame = client.result("screenshot_frame", out / "before.bmp")
    click("ControlPanelWindow.ControlPanel.TurnButton")
    after_click = client.result("query_turn")
    print(f"After Next Turn click: {after_click}", flush=True)
    client.wait_until(lambda: client.result("query_turn")["round"] == before["round"] + 1,
                      timeout=30, interval=0.1, desc="resumed Next Turn advances round")
    after = client.result("screenshot_frame", out / "after.bmp")
    assert after["frame_sequence"] > frame["frame_sequence"], (frame, after)

print("PASS resumed game accepts Next Turn and advances one round", flush=True)
