#!/usr/bin/env python3
"""Edge scrolling must yield to the main loop and accept subsequent input."""
from pathlib import Path
import sys
import ui_scenario

binary = Path(sys.argv[1]).resolve()
scenario = ui_scenario.launch(binary, "ui-edge-scroll")
out = scenario.out
print(f"Edge scroll artifacts: {out}", flush=True)
with scenario.connect() as ui:
    client = ui.client
    ui.start_new_game(seed=42, players=4)
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
