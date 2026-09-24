#!/usr/bin/env python3
"""A city-selected player can build an affordable tile improvement by clicking the map."""
from pathlib import Path
import sys

from ctp2_client import fixture_save
import ui_scenario

binary = Path(sys.argv[1]).resolve()
scenario = ui_scenario.launch(binary, "ui-tile-improvement", scrub_gpu=True)
print(f"Tile improvement artifacts: {scenario.out}", flush=True)

with scenario.connect(timeout=30) as ui:
    client = ui.client
    client.expect_ok("load_game", fixture_save("campaign-round160"))
    client.wait_game_loaded()
    client.expect_ok("debug_select_city", 0)
    ui.click_control("ControlPanelWindow.ControlPanel.ControlTabPanel.TilesTab.TabButton")
    ui.click_control("ControlPanelWindow.ControlPanel.ControlTabPanel.TilesTab.TabPanel.OceanImpButton")
    net = "ControlPanelWindow.ControlPanel.ControlTabPanel.TilesTab.TabPanel.tiOceanButtonBank.b11"
    ui.click_control(net)

    frame = client.result("screenshot_frame", scenario.out / "before.bmp")
    width, height = frame["width"], frame["height"]
    gpu = client.result("query_gpu_world")
    world = client.result("query_world")
    tile_w, tile_h = gpu["zoom_tile_wh"]
    origin_x, origin_y = gpu["worldmap_origin"]
    margin_x, margin_y = gpu["worldmap_margin"]
    assert gpu["camera"]["zoom"] == 1
    candidates = []
    for tile in client.result("query_map")["tiles"]:
        if not tile["visible"] or "city" in tile:
            continue
        x, y = tile["x"], tile["y"]
        tile_x = (x + y // 2) % world["width"]
        sx = tile_x * tile_w + (y & 1) * (tile_w // 2) - origin_x - margin_x
        sy = y * (tile_h // 2) - origin_y - margin_y
        sx += gpu["camera"]["off_x"]
        sy += gpu["camera"]["off_y"]
        sx += round((width / 2 - sx) / (world["width"] * tile_w)) * (world["width"] * tile_w)
        px, py = round(sx + tile_w // 2), round(sy + tile_h // 2)
        if 100 < px < width - 100 and 60 < py < height * 2 // 3:
            candidates.append((abs(px - width // 2) + abs(py - height // 3), x, y, px, py))
    assert candidates, "no visible map tiles away from the HUD"
    option = None
    for _, x, y, px, py in sorted(candidates)[:50]:
        details = client.result("query_terraform", x, y)
        option = next((o for o in details["options"]
                       if o["name"].lower() in {"net", "nets"} and o["affordable"]), None)
        if option:
            break
    assert option, "no affordable net tile in the visible map"
    before_materials = details["materials"]
    print(f"net target=({x},{y}) screen=({px},{py}) materials={before_materials} option={option}", flush=True)
    ui.click(px, py)
    client.result("screenshot_frame", scenario.out / "after.bmp")
    client.wait_until(
        lambda: client.result("query_terraform", x, y)["materials"] < before_materials,
        timeout=5, interval=0.05, desc="map click spends Public Works on a net")
    assert before_materials - client.result("query_terraform", x, y)["materials"] == option["cost"]
print("PASS city-selected tile improvement click spends PW and starts construction", flush=True)
