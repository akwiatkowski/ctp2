#!/usr/bin/env python3
"""A real starting settler must remain visible after map and minimap clicks.

Usage: python3 ctp2_code/test/ui_sprite_clicks.py build/ctp2
Uses SDL's windowless software renderer and only passive screenshot_frame reads.
"""

import argparse
import json
from pathlib import Path
import time

from PIL import Image

import ui_scenario
from ui_scenario import NEW_GAME, RADAR, START


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    binary = parser.parse_args().binary.resolve()
    # Keep real unit animation enabled. Only terrain must be stationary so
    # animated actor pixels can be distinguished from the empty-tile reference.
    scenario = ui_scenario.launch(
        binary, "ui-sprite-clicks",
        settings={"WaterAnim": "No", "AutoCenter": "No"}, scrub_gpu=True)
    out = scenario.out
    trace, sequence = [], 0
    step = "startup"
    print(f"Sprite click artifacts: {out}", flush=True)

    def record(event, **values):
        trace.append({"step": event, **values})
        (out / "state.json").write_text(json.dumps(trace, indent=2) + "\n")

    try:
        with scenario.connect(seed=42, players=4) as ui:
            client = ui.client
            ui.record = lambda event, **values: record(event, during=step, **values)

            def capture(name):
                nonlocal sequence
                path = out / f"{name}.bmp"
                info = {}

                def ready():
                    nonlocal info
                    response = client.command("screenshot_frame", path)
                    info = response.get("result", {})
                    return (response.get("status") == "ok"
                            and info["frame_sequence"] > sequence)

                client.wait_until(ready, timeout=10, interval=0.05,
                                  desc=f"normal frame progress: {name}")
                sequence = info["frame_sequence"]
                with Image.open(path) as source:
                    image = source.convert("RGB")
                gpu = client.result("query_gpu_world")
                record("frame", image=name, frame=info, gpu=gpu)
                return image, gpu

            def tile_origin(pos, gpu, size):
                # Invert the exact worldmap source rectangle, including wrap.
                # maputils_MapXY2WorldmapPixelXY and RenderWorldmapSpriteQuads
                # share this projection; do not assume the unit is centred.
                tile_w, tile_h = gpu["zoom_tile_wh"]
                tile_x = (pos["x"] + pos["y"] // 2) % world["width"]
                wx = tile_x * tile_w + (pos["y"] & 1) * (tile_w // 2)
                wy = pos["y"] * (tile_h // 2)
                zoom = gpu["camera"]["zoom"]
                assert zoom == 1, "scenario uses the pinned unzoomed layout"
                origin_x, origin_y = gpu["worldmap_origin"]
                margin_x, margin_y = gpu["worldmap_margin"]
                x = wx - origin_x - margin_x + gpu["camera"]["off_x"]
                y = wy - origin_y - margin_y + gpu["camera"]["off_y"]
                # Texture dimensions include right/bottom overhang tiles; the
                # wrap period is the map lattice, not that padded allocation.
                period_x = world["width"] * tile_w
                x += round((size[0] / 2 - x) / period_x) * period_x
                return round(x), round(y)

            def core(image, gpu):
                x, y = tile_origin(destination, gpu, image.size)
                # Actor hotspot is tile+(48,48). Exclude the selection diamond,
                # flag/health bar and surrounding terrain from the evidence.
                box = (x + 24, y + 10, x + 72, y + 55)
                assert 40 < box[0] < box[2] < image.width - 40, box
                assert 40 < box[1] < box[3] < image.height * 3 // 5, box
                assert not (box[0] < radar_window["x"] + radar_window["width"]
                            and box[2] > radar_window["x"]
                            and box[1] < radar_window["y"] + radar_window["height"]
                            and box[3] > radar_window["y"]), ("unit under radar", box)
                return image.crop(box)

            def difference(a, b):
                return max(abs(x - y) for x, y in zip(a, b))

            def check_unit(name):
                image, gpu = capture(name)
                army_now = next(a for a in client.result("query_armies")["armies"]
                                if a["index"] == army["index"])
                assert army_now["pos"] == destination and army_now["units"] == army["units"], army_now
                crop = core(image, gpu)
                present = sum(difference(crop.getpixel(p), empty.getpixel(p)) > 24
                              for p in mask)
                record("actor-pixels", name=name, present=present, minimum=minimum_pixels,
                       mask_pixels=len(mask), army=army_now)
                assert present >= minimum_pixels, (
                    f"{name}: settler exists at {destination} but only {present} actor "
                    f"pixels differ from empty terrain (need {minimum_pixels}); "
                    f"see {out / (name + '.bmp')}")
                return image, gpu

            step = "real menu startup"
            ui.start_new_game(seed=42, players=4)
            # Once loaded, a hung click must fail promptly rather than beachball
            # for the client's normal two-minute startup/turn allowance.
            client.sock.settimeout(10)
            radar = ui.control(RADAR)
            radar_window = ui.control("RadarWindow")
            world = client.result("query_world")
            armies = client.result("query_armies")["armies"]
            army = next(a for a in armies if a["can_settle"] and a["moves_left"] > 0)
            tiles = {(t["x"], t["y"]): t for t in client.result("query_map")["tiles"]}
            terrain = {t["id"]: t for t in client.result("query_terrains")["terrains"]}
            occupied = {(u["pos"]["x"], u["pos"]["y"])
                        for u in client.result("query_units")["units"]}
            source = army["pos"]
            step = "navigate to starting settler"
            initial_gpu = client.result("query_gpu_world")
            span_y = initial_gpu["view_rect"][3] - initial_gpu["view_rect"][1]
            # AutoCenter=No leaves startup at the map origin. Navigate normally
            # before taking the empty-tile reference; place the source in the
            # upper half of the unobscured map rather than underneath the HUD.
            center_x = (source["x"] + source["y"] // 2) % world["width"]
            center_y = min(world["height"] - 1, source["y"] + span_y // 4)
            ui.click(radar["x"] + round((center_x - 0.25 + (center_y & 1) / 2)
                                        * radar["width"] / world["width"]),
                     radar["y"] + round((center_y + 0.5)
                                        * radar["height"] / world["height"]))
            candidates = [((source["x"] + dx) % world["width"], source["y"] + dy)
                          for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1),
                                         (1, -1), (-1, 1), (1, -2), (-1, 2))]
            destination_xy = next(p for p in candidates if p in tiles
                                  and tiles[p]["visible"] and p not in occupied
                                  and terrain[tiles[p]["terrain"]]["land"])
            destination = dict(zip(("x", "y"), destination_xy))
            record("setup", army=army, destination=destination, seed=42, players=4)
            before, before_gpu = capture("empty-destination")
            empty = core(before, before_gpu)
            empty.save(out / "empty-core.png")
            step = "legitimate starting settler movement"
            moved = client.result("move_army", army["index"], *destination_xy)
            assert moved["arrived"], moved
            # Let the normal Director finish the move and fade-in; no rendering
            # command, redraw, artificial spawn or visibility manipulation.
            time.sleep(1)
            mask = set()
            counts = []
            for sample in range(8):
                settled, settled_gpu = capture(f"animated-baseline-{sample}")
                pose = core(settled, settled_gpu)
                pixels = {(x, y) for y in range(pose.height) for x in range(pose.width)
                          if difference(pose.getpixel((x, y)), empty.getpixel((x, y))) > 24}
                assert len(pixels) > 40, (
                    f"animated settler produced only {len(pixels)} actor pixels in sample {sample}")
                mask.update(pixels)
                counts.append(len(pixels))
                time.sleep(0.1)
            # Poses may change colour and silhouette. Require a substantial
            # visible body, never an exact animation frame or only a quad count.
            minimum_pixels = max(40, min(counts) // 2)
            pose.save(out / "actor-core.png")
            record("actor-mask", pixels=sorted(mask), samples=counts, minimum=minimum_pixels)
            check_unit("baseline")
            span_x = settled_gpu["view_rect"][2] - settled_gpu["view_rect"][0]
            span_y = settled_gpu["view_rect"][3] - settled_gpu["view_rect"][1]
            home_x = (settled_gpu["view_rect"][0] + span_x // 2) % world["width"]
            home_y = (settled_gpu["view_rect"][1] + span_y // 2) % world["height"]
            home = (radar["x"] + round((home_x - 0.25 + (home_y & 1) / 2)
                                      * radar["width"] / world["width"]),
                    radar["y"] + round((home_y + 0.5) * radar["height"] / world["height"]))
            home = (max(radar["x"] + 1, min(home[0], radar["x"] + radar["width"] - 2)),
                    max(radar["y"] + 1, min(home[1], radar["y"] + radar["height"] - 2)))

            for index, (fx, fy) in enumerate(((0.9, 0.9), (0.1, 0.1),
                                             (0.9, 0.1), (0.1, 0.9))):
                step = f"map selection {index}"
                image, gpu = check_unit(f"{index}-before-click")
                x, y = tile_origin(destination, gpu, image.size)
                ui.click(x + 48, y + 48)
                # Background::Idle defers a single click by doubleClickTimeout.
                # Observe the entire interval, not just the pre-click frame.
                deadline, sample = time.monotonic() + 1, 0
                while time.monotonic() < deadline:
                    check_unit(f"{index}-selected-{sample}")
                    sample += 1
                    time.sleep(0.05)
                step = f"minimap excursion {index}"
                ui.click(radar["x"] + int(fx * radar["width"]),
                         radar["y"] + int(fy * radar["height"]))
                away, away_gpu = capture(f"{index}-away")
                assert away_gpu["view_rect"] != gpu["view_rect"], away_gpu
                step = f"minimap return {index}"
                ui.click(*home)
                check_unit(f"{index}-returned")
                for sample in range(3):
                    check_unit(f"{index}-returned-{sample}")
                print(f"PASS sprite survives map/minimap clicks {index}", flush=True)
            record("passed", frame_sequence=sequence)
    except BaseException as error:
        record("failed", during=step, error=repr(error))
        raise
    finally:
        Path(scenario.socket).unlink(missing_ok=True)
    print(f"PASS actual clicks preserve passive actor pixels; {out}", flush=True)


if __name__ == "__main__":
    run()
