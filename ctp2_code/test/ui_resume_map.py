#!/usr/bin/env python3
"""Recorded regression: radar recenter must clear stale GPU camera pan."""

import os
from pathlib import Path
import sys
import tempfile

from ctp2_client import Ctp2Client, fixture_save


binary = Path(sys.argv[1]).resolve()
out = Path(tempfile.mkdtemp(prefix="ui-resume-map-", dir=binary.parent))
socket = f"/tmp/ctp2-resume-map-{os.getpid()}.sock"
env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy",
           SDL_RENDER_DRIVER="software", CTP2_CAPTURE_FRAMES="1",
           CTP2_GPU_WORLDMAP="1", CTP2_SMOKE_SOCKET=socket)
print(f"Resume map artifacts: {out}", flush=True)

with Ctp2Client(str(binary), "ui", env=env, socket_path=socket,
                log_path=str(out / "game.log"), timeout=10) as client:
    client.expect_ok("load_game", fixture_save("next-unit-freeze"))
    client.wait_game_loaded()
    frame = client.result("screenshot_frame", out / "before.bmp")

    # Accumulate a real edge-scroll camera offset, then leave the edge.
    client.expect_ok("ui_pointer", frame["width"] - 1, frame["height"] // 3, 0)
    client.wait_until(
        lambda: abs(client.result("query_gpu_world")["camera"]["off_x"]) > 5,
        timeout=5, interval=0.05, desc="camera acquires a pan offset")
    client.expect_ok("ui_pointer", frame["width"] // 2, frame["height"] // 3, 0)
    before = client.result("query_gpu_world")

    radar = client.result("ui_control_bounds", "RadarWindow.RadarMap")
    x = radar["x"] + radar["width"] // 4
    y = radar["y"] + radar["height"] // 4
    for down in (0, 1, 0):
        client.expect_ok("ui_pointer", x, y, down)

    after = client.result("query_gpu_world")
    assert abs(after["camera"]["off_x"]) < 1, (before, after)
    assert abs(after["camera"]["off_y"]) < 1, (before, after)
    shot = client.result("screenshot_frame", out / "after.bmp")
    assert shot["frame_sequence"] > frame["frame_sequence"], (frame, shot)

print("PASS radar recenter clears stale camera pan after resume", flush=True)
