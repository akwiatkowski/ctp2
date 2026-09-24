#!/usr/bin/env python3
"""Recorded session 20260923T163745Z: Next Unit froze in synchronous Refresh.

The promoted turn-1 checkpoint has Rome plus one settler. Replaying two real
Next button clicks must remain responsive and produce a subsequent normal frame.
"""

from pathlib import Path
import sys
import time

from PIL import Image, ImageChops
from ctp2_client import fixture_save
import ui_scenario


binary = Path(sys.argv[1]).resolve()
scenario = ui_scenario.launch(binary, "ui-next-unit",
                              settings={"ShowCityNames": "Yes"})
out = scenario.out
print(f"Next Unit artifacts: {out}", flush=True)


def click(client, path):
    bounds = client.result("ui_control_bounds", path)
    assert bounds["visible"] and bounds["enabled"], (path, bounds)
    x = bounds["x"] + bounds["width"] // 2
    y = bounds["y"] + bounds["height"] // 2
    for down in (0, 1, 0):
        client.expect_ok("ui_pointer", x, y, down)


with scenario.connect(timeout=5) as ui:
    client = ui.client
    client.expect_ok("load_game", fixture_save("next-unit-freeze"))
    client.wait_game_loaded()

    started = time.monotonic()
    click(client, "ControlPanelWindow.ControlPanel.ControlTabPanel.UnitTab.TabButton")
    baseline_elapsed = time.monotonic() - started
    next_button = "ControlPanelWindow.ControlPanel.ControlTabPanel.UnitTab.TabPanel.UnitSelectionDisplay.UnitSelect.Next"
    click(client, "ControlPanelWindow.ControlPanel.ControlTabPanel.UnitTab.TabPanel.UnitOrderButtonGrid.Order0")
    before = client.result("screenshot_frame", out / "before.bmp")
    started = time.monotonic()
    click(client, next_button)
    first_elapsed = time.monotonic() - started
    started = time.monotonic()
    click(client, next_button)
    second_elapsed = time.monotonic() - started
    print(f"Tab baseline: {baseline_elapsed:.3f}s; Next Unit: "
          f"{first_elapsed:.3f}s, {second_elapsed:.3f}s", flush=True)
    client.result("query_turn")
    after = client.result("screenshot_frame", out / "after.bmp")
    assert after["frame_sequence"] > before["frame_sequence"], (before, after)

with scenario.connect(timeout=5, log_name="hoplite.log") as ui:
    client = ui.client
    client.expect_ok("load_game", fixture_save("hoplite-move-freeze"))
    client.wait_game_loaded()
    vision = client.result("debug_vision_stats", 16, 60, 20)
    assert vision["fog_snapshots"]["present"] > 0, vision
    assert vision["fog_snapshots"]["tile_mismatches"] == 0, vision
    hoplite = client.result("query_armies")["armies"][0]
    click(client, "ControlPanelWindow.ControlPanel.ControlTabPanel.UnitTab.TabButton")
    click(client, next_button)
    assert hoplite["pos"] == {"x": 17, "y": 57}, hoplite
    hoplite_before = client.result("screenshot_frame", out / "hoplite-before.bmp")
    client.expect_ok("debug_set_grid", 1)
    client.expect_ok("debug_set_grid", 0)
    client.expect_ok("debug_reveal_patch", 17, 57, 0)
    rebuilt_path = out / "hoplite-rebuilt.bmp"
    client.result("screenshot_frame", rebuilt_path)
    initial = Image.open(out / "hoplite-before.bmp").convert("RGB")
    rebuilt = Image.open(rebuilt_path).convert("RGB")
    terrain_box = (0, 30, initial.width, initial.height * 3 // 4)
    diff = ImageChops.difference(initial.crop(terrain_box), rebuilt.crop(terrain_box))
    changed = sum(max(pixel) > 20 for pixel in diff.get_flattened_data())
    assert changed / (diff.width * diff.height) < 0.01, (
        f"restored fog texture changed after forced rebuild: {changed} pixels")
    moved = client.result("move_army", hoplite["index"], 15, 60)
    assert not moved["arrived"], moved
    hoplite_after = client.result("query_armies")["armies"][0]
    assert hoplite_after["pos"] != hoplite["pos"], (hoplite, hoplite_after)
    hoplite_frame = client.result("screenshot_frame", out / "hoplite-after.bmp")
    assert hoplite_frame["frame_sequence"] > hoplite_before["frame_sequence"]
    client.result("query_turn")
    client.expect_ok("debug_select_city", 0)
    click(client, "ControlPanelWindow.ControlPanel.ControlTabPanel.CityTab.TabButton")
    build = client.result(
        "ui_control_bounds",
        "ControlPanelWindow.ControlPanel.ControlTabPanel.CityTab.TabPanel.BuildProgress.IconBorder.IconButton")
    assert build["visible"] and build["enabled"], build
    city_frame = client.result("screenshot_frame", out / "city-tab.bmp")
    assert city_frame["frame_sequence"] > hoplite_frame["frame_sequence"]
    radar = client.result("ui_control_bounds", "RadarWindow.RadarMap")
    for down in (0, 1, 0):
        client.expect_ok("ui_pointer", radar["x"] + radar["width"] // 2,
                         radar["y"] + 1, down)
    north = client.result("query_gpu_world")["view_rect"]
    assert north[1] < 0, north
    north_path = out / "north-edge.bmp"
    client.wait_until(
        lambda: client.result("screenshot_frame", north_path)["frame_sequence"]
                > city_frame["frame_sequence"],
        timeout=5, interval=0.05, desc="north-edge frame follows radar click")
    north_frame = Image.open(north_path).convert("RGB")
    # At this radar north-edge pose, y=80 is below the 30px menu but above row 0.
    backdrop = north_frame.getpixel((north_frame.width // 2, 80))
    assert 0 < sum(backdrop) < 60, (backdrop, client.result("query_gpu_world"))
    client.expect_ok("camera_debug_layers", out / "north-world.bmp", out / "north-ui.bmp")
    ui_layer = Image.open(out / "north-ui.bmp").convert("RGBA")
    # The top of the control-panel rectangle is above its visible chrome.
    # In the pinned 1024x768 layout, (700,585) is a stencil hole: only the
    # world layer may supply pixels there, even after moving the view north.
    assert ui_layer.getpixel((700, 585))[3] == 0, (
        "map pixels baked into transparent control-panel UI", out / "north-ui.bmp")
    # Just below that hole the panel artwork must remain opaque and visible.
    chrome = ui_layer.getpixel((700, 620))
    assert chrome[3] == 255 and sum(chrome[:3]) > 200, ("missing HUD", chrome)

print("PASS recorded Next Unit sequence remains responsive", flush=True)
