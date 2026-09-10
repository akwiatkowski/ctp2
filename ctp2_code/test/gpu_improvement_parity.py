#!/usr/bin/env python3
"""Compare GPU improvement shadows and both border styles to CPU cell tiles."""
import os
from pathlib import Path
import sys
import tempfile
import time

from ctp2_client import Ctp2Client
from raster_overlay_parity import crop_pixels


def differences(a, b):
    return sum(pa != pb for ra, rb in zip(a, b) for pa, pb in zip(ra, rb))


def capture(binary, enabled, directory):
    tag = "gpu" if enabled else "cpu"
    env = os.environ.copy()
    env.update(CTP2_GPU_LAYERS="1", CTP2_GPU_CAMERA="1", CTP2_GPU_QUADS="1",
               CTP2_GPU_WORLDMAP="1", CTP2_GPU_RASTER=str(int(enabled)))
    socket = str(directory / f"{tag}.sock")
    env["CTP2_SMOKE_SOCKET"] = socket
    images = {}
    with Ctp2Client(binary, "ui", socket_path=socket, env=env,
                    log_path=str(directory / f"{tag}.log")) as c:
        fixture = directory / "scene.json"
        if enabled:
            c.expect_ok("load_game", fixture)
        else:
            c.expect_ok("new_game")
            c.expect_ok("start_game")
            c.wait_game_loaded()
            c.expect_ok("build_city")
            c.expect_ok("save_game", fixture)
            c.expect_ok("load_game", fixture)
        # Founding/loading may open this dialog over the camera centre.
        c.expect_ok("debug_close_build_manager")
        c.expect_ok("debug_deselect")
        c.expect_ok("debug_worldmap_sprites", 0)
        c.expect_ok("set_show_city_names", 0)
        c.expect_ok("debug_set_grid", 0)
        original = c.result("debug_set_borders", 0, 1)["was"]
        city = c.result("query_cities")["cities"][0]["pos"]
        terrains = c.result("query_terrains")["terrains"]
        grass = next(t["id"] for t in terrains if "grassland" in t["internal"].lower())
        names = c.result("query_names")["terrain_improvements"]
        types = []
        for name in ("farm", "mine", "road"):
            candidates = [(int(i), text) for i, text in names.items()
                          if text.lower().replace("tileimp_", "") == name]
            assert candidates, f"missing {name}: {names}"
            types.append(candidates[0][0])

        def shot(name, center):
            path = directory / f"{tag}-{name}.bmp"
            last = None
            for _ in range(25):
                c.expect_ok("camera_debug_center", *center)
                c.expect_ok("screenshot_presented", path)
                pixels = crop_pixels(path)
                if pixels == last:
                    images[name] = pixels
                    return
                last = pixels
                time.sleep(0.1)
            raise AssertionError(f"frame did not settle: {name}")

        try:
            center = (city["x"], city["y"])
            world = c.result("query_map")
            c.expect_ok("debug_reveal_patch", *center, 14)
            cells = [((center[0] + dx) % world["width"], (center[1] + dy) % world["height"])
                     for dy in range(-2, 3) for dx in range(-2, 3)]
            for x, y in cells:
                c.expect_ok("debug_set_terrain", x, y, grass)
            shot("bare", center)
            for index, (x, y) in enumerate(cells):
                c.expect_ok("debug_place_improvement", x, y, types[index % len(types)])
            shot("improvements", center)
            assert differences(images["bare"], images["improvements"]) > 40, "empty improvement fixture"
            for name, smooth in (("smooth", 1), ("line", 0)):
                c.expect_ok("debug_set_borders", 1, smooth)
                shot(name, center)
                assert differences(images["improvements"], images[name]) > 40, f"empty {name} borders"
            c.expect_ok("debug_set_grid", 1)
            shot("grid", center)
            stats = c.result("debug_worldmap_build", "rebuild")
            print(tag, stats, flush=True)
            if enabled:
                assert stats["raster_cells"] > 0, "GPU path never used"
            census = c.result("debug_tileset_stats")
            assert census["improvements"]["shadow_runs"] > 0, "no shadow coverage"
            print(tag, "overlay census:", census["improvements"], flush=True)
        finally:
            c.expect_ok("debug_set_borders", original["borders"], original["smooth"])
    return images


def main():
    binary = str(Path(sys.argv[1]).resolve())
    directory = Path(tempfile.mkdtemp(prefix="ctp2-improvement-parity-"))
    print("Artifacts:", directory, flush=True)
    reference = capture(binary, False, directory)
    actual = capture(binary, True, directory)
    failures = []
    for name, expected in reference.items():
        count = differences(expected, actual[name])
        print(f"{name}: {count} differing pixels", flush=True)
        if count:
            failures.append(name)
    assert not failures, f"GPU/CPU parity failures: {failures}"


if __name__ == "__main__":
    main()
