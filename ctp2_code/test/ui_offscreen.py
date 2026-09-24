#!/usr/bin/env python3
"""Exercise real menu/map input and passive frames without a window or audio.

Usage: python3 ctp2_code/test/ui_offscreen.py build/ctp2
Requires Pillow, like the other UI rendering scenarios. Artifacts are retained
in a unique ui-offscreen-* directory beside the binary, including on failure.
"""
import argparse
import json
import math
from pathlib import Path
import sys

from PIL import Image

import ui_scenario
from ui_scenario import NEW_GAME, RADAR, START

# Reuse the sampled-content scale from worldmap_load_renders.py, but only
# inside the map ROI. Require texture variation too, not a solid bright panel.
SAMPLE_STEP = 8
LIT_RGB_SUM = 30
MIN_TERRAIN_SAMPLES = 60
MIN_TERRAIN_COLORS = 12  # Test acceptance policy: more than a few flat UI colors.


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    args = parser.parse_args()
    binary = args.binary.resolve()
    # An inherited developer opt-out must not silently select a legacy renderer.
    scenario = ui_scenario.launch(binary, "ui-offscreen", scrub_gpu=True,
                                  extra_env={"CTP2_GPU_WORLDMAP": "1"})
    out = scenario.out
    seed, players = 42, 4
    trace = []
    sequence = 0
    step = "startup"
    print(f"Offscreen UI artifacts: {out}", flush=True)

    def record(name, **values):
        trace.append({"step": name, **values})
        (out / "state.json").write_text(json.dumps(trace, indent=2) + "\n")

    record(step, binary=str(binary), seed=seed, players=players,
           profile=scenario.env["CTP2_PROFILE"], socket=scenario.socket)
    try:
        with scenario.connect(seed=seed, players=players) as ui:
            client = ui.client
            ui.record = record

            def capture(name, terrain=False):
                nonlocal sequence
                path = out / f"{name}.bmp"
                previous = sequence
                info, samples = {}, {}

                def ready():
                    nonlocal info, samples, sequence
                    response = client.command("screenshot_frame", path)
                    info = response.get("result", {})
                    if response.get("status") != "ok":
                        return False
                    sequence = info["frame_sequence"]
                    if sequence <= previous:
                        return False
                    with Image.open(path) as source:
                        image = source.convert("RGB")
                    assert image.size == (info["width"], info["height"]), info
                    if not terrain:
                        return True
                    # Pinned 1024x768 layout: exclude top menus, side edges and
                    # the entire lower HUD/radar, never count HUD as terrain.
                    roi = (40, 40, image.width - 40, image.height * 3 // 5)
                    pixels = image.load()
                    lit = [(x, y) for y in range(roi[1], roi[3], SAMPLE_STEP)
                           for x in range(roi[0], roi[2], SAMPLE_STEP)
                           if not (radar_window["x"] <= x < radar_window["x"] + radar_window["width"]
                                   and radar_window["y"] <= y < radar_window["y"] + radar_window["height"])
                           if sum(pixels[x, y]) >= LIT_RGB_SUM]
                    colors = {pixels[x, y] for x, y in lit}
                    samples = {"roi": roi, "lit_samples": len(lit),
                               "colors": len(colors)}
                    if len(lit) < MIN_TERRAIN_SAMPLES or len(colors) < MIN_TERRAIN_COLORS:
                        return False
                    # Hover actual visible content, not unexplored black fog.
                    samples["hover"] = min(lit, key=lambda p:
                        (p[0] - image.width // 2) ** 2
                        + (p[1] - image.height // 2) ** 2)
                    return True

                try:
                    client.wait_until(ready, timeout=30, interval=0.05,
                                      desc=f"new normal frame {name}"
                                      + (" with map terrain" if terrain else ""))
                finally:
                    record("frame", image=name, frame=info, samples=samples)
                return samples

            def view_state(name):
                status = client.result("query_turn")
                gpu = client.result("query_gpu_world")
                record(name, status=status, gpu=gpu)
                view = gpu.get("view_rect", [])
                assert (len(view) == 4 and all(isinstance(v, int) for v in view)
                        and 0 < view[2] - view[0] <= world["width"]
                        and 0 < view[3] - view[1] <= world["height"]), gpu
                assert gpu.get("worldmap") and gpu.get("worldmap_texture"), gpu
                assert all(math.isfinite(v) for v in gpu["camera"].values()), gpu
                assert gpu["camera"]["zoom"] > 0, gpu
                return view

            try:
                step = "new-game menu click"
                capture("main-menu")
                ui.click_control(NEW_GAME)
                step = "start-game setup click"
                ui.control(START)
                client.expect_ok("ui_prepare_game", seed, players)
                capture("setup")
                ui.click_control(START)
                client.wait_game_loaded(timeout=120)
                radar = ui.control(RADAR)
                radar_window = ui.control("RadarWindow")
                world = client.result("query_world")
                armies = client.result("query_armies")["armies"]
                assert armies, "new game has no human starting armies"
                assert all(0 <= a["pos"]["x"] < world["width"]
                           and 0 <= a["pos"]["y"] < world["height"]
                           for a in armies), armies
                record("world", width=world["width"], height=world["height"],
                       armies=armies)
                step = "natural initial terrain"
                visible = capture("spawn", terrain=True)
                initial = view_state("initial-view")
                span_x, span_y = initial[2] - initial[0], initial[3] - initial[1]
                # Radar uses skewed tile coordinates, exactly like view_rect.
                # Its display offset starts at zero and only right-click moves
                # it; this scenario sends left clicks exclusively.
                tile_x = (initial[0] + span_x // 2) % world["width"]
                tile_y = (initial[1] + span_y // 2) % world["height"]
                home = (radar["x"] + round((tile_x - 0.25 + (tile_y & 1) / 2)
                                           * radar["width"] / world["width"]),
                        radar["y"] + round((tile_y + 0.5)
                                           * radar["height"] / world["height"]))
                home = (max(radar["x"] + 1, min(home[0], radar["x"] + radar["width"] - 2)),
                        max(radar["y"] + 1, min(home[1], radar["y"] + radar["height"] - 2)))
                shifted_right = shifted_down = False
                before = initial
                for index, (fx, fy) in enumerate(((0.9, 0.9), (0.1, 0.1),
                                                 (0.9, 0.1), (0.1, 0.9))):
                    step = f"minimap excursion {index}"
                    client.expect_ok("ui_pointer", *visible["hover"], 0)
                    # Entering the map fires MouseMoveOver; a second movement
                    # inside it fires MouseMoveInside and enables the tile cursor.
                    client.expect_ok("ui_pointer", visible["hover"][0] + 1, visible["hover"][1], 0)
                    capture(f"{index}-hover")
                    ui.click(radar["x"] + int(fx * radar["width"]),
                             radar["y"] + int(fy * radar["height"]))
                    client.wait_until(lambda: client.result("query_gpu_world")["view_rect"] != before,
                                      timeout=10, interval=0.05,
                                      desc=f"minimap {index} changes view")
                    away = view_state(f"{index}-away-view")
                    shifted_right |= away[0] - before[0] > span_x // 2
                    shifted_down |= away[1] - before[1] > span_y // 2
                    capture(f"{index}-away")  # Black unexplored fog is legitimate.
                    step = f"minimap return {index}"
                    ui.click(*home)
                    client.wait_until(lambda: client.result("query_gpu_world")["view_rect"] != away,
                                      timeout=10, interval=0.05,
                                      desc=f"minimap {index} returns to visible map")
                    returned = view_state(f"{index}-return-view")
                    shifted_right |= returned[0] - away[0] > span_x // 2
                    shifted_down |= returned[1] - away[1] > span_y // 2
                    client.expect_ok("ui_pointer", *visible["hover"], 0)
                    visible = capture(f"{index}-returned", terrain=True)
                    before = returned
                    print(f"PASS minimap {index}: {away} -> {returned}", flush=True)
                assert shifted_right and shifted_down, (
                    "scenario did not move the previous highlight left/above the viewport")
                record("passed", frame_sequence=sequence)
            except BaseException as error:
                record("failed", during=step, error=repr(error))
                # Preserve the last NORMAL frame even if input crashed before
                # the next present; this never repairs or invalidates rendering.
                if client.proc.poll() is None:
                    try:
                        record("failure-frame", response=client.command(
                            "screenshot_frame", out / "failure.bmp"))
                    except Exception as diagnostic_error:
                        record("diagnostics-failed", error=repr(diagnostic_error))
                raise
    except BaseException:
        print(f"FAIL during {step}; see {out / 'state.json'} and {out / 'game.log'}",
              file=sys.stderr, flush=True)
        raise
    finally:
        Path(scenario.socket).unlink(missing_ok=True)
    print(f"PASS offscreen full-game UI: real clicks and passive terrain frames; {out}")


if __name__ == "__main__":
    run()
