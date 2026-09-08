#!/usr/bin/env python3
"""Units must refresh on the GPU without a camera/terrain refresh hiding bugs."""

import os
from pathlib import Path
import sys

from ctp2_client import Ctp2Client
from gpu_world_fallbacks import _bmp_dims, _bmp_pixel


def run(binary):
    artifacts = Path(binary).resolve().parent / "gpu-sprite-refresh"
    artifacts.mkdir(exist_ok=True)
    env = dict(os.environ, CTP2_GPU_LAYERS="1", CTP2_GPU_CAMERA="1",
               CTP2_GPU_QUADS="1", CTP2_GPU_WORLDMAP="1", CTP2_GPU_RASTER="1",
               CTP2_MODERN_SPRITES="1")
    socket = f"/tmp/ctp2-sprite-refresh-{os.getpid()}.sock"
    env["CTP2_SMOKE_SOCKET"] = socket
    with Ctp2Client(str(Path(binary).resolve()), "ui", env=env, socket_path=socket,
                    log_path=str(artifacts / "game.log")) as client:
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded()
        city = client.result("build_city")["pos"]
        client.expect_ok("debug_close_build_manager")
        client.expect_ok("debug_deselect")
        client.expect_ok("set_show_city_names", 0)
        client.expect_ok("debug_reveal_patch", city["x"], city["y"], 20)
        client.expect_ok("debug_clear_terrain_layers", city["x"], city["y"], 60)
        world = client.result("query_map")
        city_tile = next(t for t in world["tiles"] if (t["x"], t["y"]) == (city["x"], city["y"]))
        destination = {"x": (city["x"] + 1) % world["width"], "y": city["y"]}
        client.expect_ok("debug_set_terrain", destination["x"], destination["y"], city_tile["terrain"])
        for army in reversed(client.result("query_armies")["armies"]):
            if army["pos"] == city:
                client.expect_ok("disband_unit", army["index"])
        client.expect_ok("camera_debug_center", city["x"], city["y"])

        def capture(name):
            path = artifacts / f"{name}.bmp"
            client.expect_ok("screenshot_presented", str(path))
            return path.read_bytes()

        baseline = client.result("query_gpu_world")
        assert baseline["worldmap"] and baseline["worldmap_texture"]
        before = capture("city")
        created = client.result("create_unit", "UNIT_ARCHER", city["x"], city["y"])
        # No centering, Refresh, or render-fixture command from here onwards.
        client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] > baseline["sprite_quads"],
                          timeout=5, desc="unit in city appears without terrain refresh")
        after = capture("city-with-unit")
        width, height = _bmp_dims(after)
        original_pixels = [(x, y) for y in range(40, height - 200)
                           for x in range(300, width - 20)
                           if _bmp_pixel(before, x, y) != _bmp_pixel(after, x, y)]
        changed = len(original_pixels)
        assert changed > 20, f"unit produced no visible map pixels ({changed})"
        print(f"PASS: unit in city appears with a stationary camera ({changed} pixels)", flush=True)

        armies = client.result("query_armies")["armies"]
        army = next(a for a in armies if a["pos"] == city
                    and any(u["type"] == created["type"] for u in a["units"]))
        moved = client.result("move_army", army["index"], destination["x"], destination["y"])
        assert moved["arrived"], moved

        def movement_visible():
            frame = capture("unit-moved")
            # Movement must clear a substantial part of the original unit image.
            return sum(_bmp_pixel(frame, x, y) == _bmp_pixel(before, x, y)
                       for x, y in original_pixels) > len(original_pixels) // 2

        client.wait_until(movement_visible, timeout=5, desc="moving unit clears its old position")
        moved_frame = capture("unit-moved")
        assert client.result("query_gpu_world")["sprite_quads"] > baseline["sprite_quads"]
        print("PASS: movement updates the sprite position with a stationary camera", flush=True)
        client.expect_ok("disband_unit", army["index"])
        client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] == baseline["sprite_quads"],
                          timeout=5, desc="disbanded unit disappears without terrain refresh")
        removed = capture("city-after-disband")
        restored = sum(_bmp_pixel(moved_frame, x, y) != _bmp_pixel(removed, x, y)
                       for y in range(40, height - 200) for x in range(300, width - 20))
        assert restored > 20, f"disband left stale unit pixels ({restored})"
        final = client.result("query_gpu_world")
        assert final["camera"] == baseline["camera"]
        assert final["worldmap_origin"] == baseline["worldmap_origin"]
        assert final["complete"], final["fallback_reason"]
        print("PASS: disband removes the sprite without camera movement", flush=True)
    print(f"Artifacts: {artifacts}")


if __name__ == "__main__":
    run(sys.argv[1])
