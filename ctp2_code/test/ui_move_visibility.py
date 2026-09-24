#!/usr/bin/env python3
"""Moving a real unit must repaint newly visible terrain without camera input."""

from pathlib import Path
import sys

from PIL import Image

from ctp2_client import fixture_save
import ui_scenario


PIXEL_SAMPLE_STEP = 4
# Recorded turn-1 fixture exposes eight 4px-grid samples; zero was the bug.
MIN_REVEALED_SAMPLES = 5

binary = Path(sys.argv[1]).resolve()
scenario = ui_scenario.launch(binary, "ui-move-visibility",
                              extra_env={"CTP2_GPU_WORLDMAP": "1"})
out = scenario.out
print(f"Move visibility artifacts: {out}", flush=True)

with scenario.connect(timeout=10) as ui:
    client = ui.client
    client.expect_ok("load_game", fixture_save("next-unit-freeze"))
    client.wait_game_loaded()
    world_before = client.result("query_map")
    known_before = {(tile["x"], tile["y"]) for tile in world_before["tiles"]}
    gpu_before = client.result("query_gpu_world")
    before_path = out / "before.bmp"
    before_info = client.result("screenshot_frame", before_path)
    before = Image.open(before_path).convert("RGB")

    army = client.result("query_armies")["armies"][0]
    assert army["pos"] == {"x": 2, "y": 87}, army
    moved = client.result("move_army", army["index"], 3, 86)
    assert moved["arrived"], moved
    client.wait_until(
        lambda: len(client.result("query_map")["tiles"]) > len(known_before),
        timeout=5, interval=0.05, desc="unit movement reveals new terrain state")
    world_after = client.result("query_map")
    newly_visible = [tile for tile in world_after["tiles"]
                     if (tile["x"], tile["y"]) not in known_before]
    assert newly_visible, "movement exposed no new terrain"
    client.wait_until(
        lambda: client.result("query_gpu_world")["last_terrain_build"]["submitted_quads"] > 0,
        timeout=5, interval=0.05, desc="vision change rebuilds whole-map terrain")

    after_path = out / "after.bmp"
    info = {}
    def fresh_frame():
        response = client.command("screenshot_frame", after_path)
        if response.get("status") != "ok":
            return False
        info.update(response["result"])
        return info["frame_sequence"] > before_info["frame_sequence"]
    client.wait_until(fresh_frame, timeout=5, interval=0.05,
                      desc="movement produces a fresh passive frame")
    after = Image.open(after_path).convert("RGB")
    gpu_after = client.result("query_gpu_world")

    def tile_origin(tile, gpu):
        tile_w, tile_h = gpu["zoom_tile_wh"]
        tile_x = (tile["x"] + tile["y"] // 2) % world_after["width"]
        wx = tile_x * tile_w + (tile["y"] & 1) * (tile_w // 2)
        wy = tile["y"] * (tile_h // 2)
        x = wx - gpu["worldmap_origin"][0] + gpu["camera"]["off_x"]
        y = wy - gpu["worldmap_origin"][1] + gpu["camera"]["off_y"]
        period = world_after["width"] * tile_w
        x += round((after.width / 2 - x) / period) * period
        return round(x), round(y)

    def newly_lit_samples(old, new):
        count = 0
        for tile in newly_visible:
            x, y = tile_origin(tile, gpu_after)
            for py in range(max(40, y + 20), min(new.height * 3 // 5, y + 72),
                            PIXEL_SAMPLE_STEP):
                for px in range(max(40, x), min(new.width - 40, x + 96),
                                PIXEL_SAMPLE_STEP):
                    if sum(old.getpixel((px, py))) < 30 <= sum(new.getpixel((px, py))):
                        count += 1
        return count

    revealed_pixels = newly_lit_samples(before, after)
    assert revealed_pixels >= MIN_REVEALED_SAMPLES, (
        f"new vision stayed black after movement: {revealed_pixels} pixels; "
        f"tiles={newly_visible}; gpu_before={gpu_before}; gpu_after={gpu_after}")

    # This explicit rebuild models the user's camera nudge. It must not reveal
    # terrain that the passive post-movement frame left black.
    client.expect_ok("debug_reveal_patch", 3, 86, 0)
    refreshed_path = out / "refreshed.bmp"
    refreshed_info = client.result("screenshot_frame", refreshed_path)
    refreshed = Image.open(refreshed_path).convert("RGB")
    late_reveals = newly_lit_samples(after, refreshed)
    assert late_reveals == 0, (
        f"camera-triggered rebuild revealed {late_reveals} terrain samples that "
        f"movement left black; tiles={newly_visible}")

print(f"PASS unit movement repaints newly visible terrain ({revealed_pixels} samples)", flush=True)
