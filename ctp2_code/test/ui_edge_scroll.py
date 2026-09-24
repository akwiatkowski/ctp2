#!/usr/bin/env python3
"""Edge scrolling must yield to the main loop and accept subsequent input."""
import os
from pathlib import Path
import sys
import tempfile
from ctp2_client import Ctp2Client

binary = Path(sys.argv[1]).resolve()
out = Path(tempfile.mkdtemp(prefix="ui-edge-scroll-", dir=binary.parent))
socket = f"/tmp/ctp2-edge-{os.getpid()}.sock"
env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy",
           SDL_RENDER_DRIVER="software", CTP2_CAPTURE_FRAMES="1", CTP2_SMOKE_SOCKET=socket)
print(f"Edge scroll artifacts: {out}", flush=True)
with Ctp2Client(str(binary), "ui", env=env, socket_path=socket,
                log_path=str(out / "game.log")) as client:
    def click(path):
        b = client.result("ui_control_bounds", path)
        assert b["visible"] and b["enabled"], b
        for down in (0, 1, 0):
            client.expect_ok("ui_pointer", b["x"] + b["width"] // 2,
                             b["y"] + b["height"] // 2, down)
    click("InitPlayWindow.NewGameButton")
    client.expect_ok("ui_prepare_game", 42, 4)
    click("SPNewGameWindow.StartButton")
    client.wait_game_loaded()
    client.sock.settimeout(5)  # A main-loop input round trip must not monopolize a frame.
    before = client.result("query_gpu_world")
    frame = client.result("screenshot_frame", out / "before.bmp")
    # Stay above the lower HUD. One pixel inside the right edge triggers scrolling.
    client.expect_ok("ui_pointer", frame["width"] - 1, frame["height"] // 3, 0)
    try:
        client.wait_until(lambda: client.result("query_gpu_world")["camera"] != before["camera"],
                          timeout=5, interval=0.05, desc="edge scroll camera advances")
        client.wait_until(
            lambda: client.result("query_gpu_world").get("radar_view_rect")
                    != before.get("radar_view_rect"),
            timeout=2, interval=0.05, desc="radar rectangle follows GPU camera")
        client.expect_ok("ui_pointer", frame["width"] // 2, frame["height"] // 3, 0)
        client.result("query_turn")
        after = client.result("screenshot_frame", out / "after.bmp")
        assert after["frame_sequence"] > frame["frame_sequence"]
    finally:
        client.sock.settimeout(1)
print("PASS edge scroll advances camera and remains responsive to input", flush=True)
