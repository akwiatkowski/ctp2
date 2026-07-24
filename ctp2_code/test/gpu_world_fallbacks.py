#!/usr/bin/env python3
"""Regression checks for GPU-world fallback reasons that must stay honest."""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402


def _bmp_pixel(data, x, y):
    off = struct.unpack_from("<I", data, 10)[0]
    w = struct.unpack_from("<i", data, 18)[0]
    h = struct.unpack_from("<i", data, 22)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]
    assert bpp in (24, 32), f"unexpected BMP bpp {bpp}"
    row = (h - 1 - y) if h > 0 else y
    rowsize = ((bpp * w + 31) // 32) * 4
    p = off + row * rowsize + x * (bpp // 8)
    b, g, r = data[p], data[p + 1], data[p + 2]
    return (r, g, b)


def _bmp_dims(data):
    return (struct.unpack_from("<i", data, 18)[0],
            abs(struct.unpack_from("<i", data, 22)[0]))


def changed_samples(path_a, path_b):
    with open(path_a, "rb") as f:
        a = f.read()
    with open(path_b, "rb") as f:
        b = f.read()
    aw, ah = _bmp_dims(a)
    bw, bh = _bmp_dims(b)
    assert (aw, ah) == (bw, bh), f"size mismatch: {aw}x{ah} vs {bw}x{bh}"
    return sum(
        1
        for y in range(0, ah, 2)
        for x in range(0, aw, 2)
        if _bmp_pixel(a, x, y) != _bmp_pixel(b, x, y)
    )


def expect_visible_change(label, path_a, path_b, minimum=1):
    changed = changed_samples(path_a, path_b)
    assert changed >= minimum, f"{label} changed only {changed} sampled pixels"
    print(f"[gpu-fallbacks] PASS: {label} changed {changed} sampled pixels")


def run(binary):
    env = os.environ.copy()
    env["CTP2_GPU_LAYERS"] = "1"
    env["CTP2_GPU_CAMERA"] = "1"
    env["CTP2_GPU_QUADS"] = "1"
    run_id = os.getpid()
    socket_path = f"/tmp/ctp2-gpu-fallbacks-{run_id}.sock"
    env["CTP2_SMOKE_SOCKET"] = socket_path
    log = f"/tmp/ctp2_gpu_fallbacks_{run_id}.log"
    screenshot = f"/tmp/ctp2_gpu_fallbacks_{run_id}.bmp"
    before = f"/tmp/ctp2_gpu_fallbacks_{run_id}_before.bmp"
    after = f"/tmp/ctp2_gpu_fallbacks_{run_id}_after.bmp"
    for path in (screenshot, before, after):
        try:
            os.unlink(path)
        except FileNotFoundError:
            pass

    try:
        with Ctp2Client(binary, "ui", socket_path=socket_path, log_path=log, env=env) as client:
            client.expect_ok("new_game")
            client.expect_ok("start_game")
            client.wait_game_loaded()

            city = client.result("build_city")["pos"]
            client.expect_ok("set_show_city_names", 0)
            client.expect_ok("camera_debug_center", city["x"], city["y"])
            client.expect_ok("screenshot_presented", screenshot)
            gpu = client.result("query_gpu_world")
            assert gpu["enabled"] and gpu["complete"], gpu
            print("[gpu-fallbacks] PASS: city actor base renders on GPU")

            created = client.result("create_unit", "UNIT_ARCHER", city["x"], city["y"])
            armies = client.result("query_armies")["armies"]
            archer_army = next(
                a for a in armies
                if a["pos"] == city and any(u["type"] == created["type"] for u in a["units"])
            )
            client.expect_ok("fortify", archer_army["index"])
            client.expect_ok("camera_debug_center", city["x"], city["y"])
            client.expect_ok("screenshot_presented", screenshot)
            gpu = client.result("query_gpu_world")
            assert gpu["enabled"] and gpu["complete"], gpu
            print("[gpu-fallbacks] PASS: fortified unit state stays on GPU path")

            client.expect_ok("end_turn", 1)
            client.expect_ok("camera_debug_center", city["x"], city["y"])
            client.expect_ok("screenshot_presented", screenshot)
            gpu = client.result("query_gpu_world")
            assert gpu["enabled"] and gpu["complete"], gpu
            print("[gpu-fallbacks] PASS: end_turn keeps GPU path complete")

            client.expect_ok("set_show_city_names", 0)
            client.expect_ok("screenshot_presented", before)
            client.expect_ok("set_show_city_names", 1)
            last_gpu = {}

            def city_names_complete():
                nonlocal last_gpu
                r = client.command("camera_debug_center", city["x"], city["y"])
                if r.get("status") != "ok":
                    if r.get("detail") == "modal":
                        return False
                    raise Ctp2Error(f"camera_debug_center failed: {r}")
                client.expect_ok("screenshot_presented", screenshot)
                gpu = client.result("query_gpu_world")
                last_gpu = gpu
                return gpu["enabled"] and gpu["complete"]

            try:
                client.wait_until(city_names_complete, timeout=60, desc="city-name GPU render")
            except Ctp2Error:
                print(f"[gpu-fallbacks] observed gpu state: {last_gpu}")
                raise
            print("[gpu-fallbacks] PASS: city names stay on GPU path")
            client.expect_ok("screenshot_presented", after)
            expect_visible_change("city names", before, after)

            client.expect_ok("set_show_city_names", 0)
            client.expect_ok("camera_debug_center", city["x"], city["y"])
            client.expect_ok("screenshot_presented", before)
            client.expect_ok("debug_terrain_overlay", city["x"] + 1, city["y"])
            last_gpu = {}

            def terrain_overlay_visible():
                nonlocal last_gpu
                client.expect_ok("camera_debug_center", city["x"], city["y"])
                client.expect_ok("screenshot_presented", after)
                gpu = client.result("query_gpu_world")
                last_gpu = gpu
                assert gpu["enabled"] and gpu["complete"], gpu
                return changed_samples(before, after) > 0

            try:
                client.wait_until(terrain_overlay_visible, timeout=10, desc="terrain overlay visible")
            except Ctp2Error:
                print(f"[gpu-fallbacks] observed gpu state: {last_gpu}")
                raise
            print("[gpu-fallbacks] PASS: terrain overlay flag stays on GPU path")
            expect_visible_change("terrain overlay", before, after)

            client.expect_ok("screenshot_presented", before)
            client.expect_ok("set_zoom_level", 4)
            client.expect_ok("camera_debug_center", city["x"], city["y"])
            client.expect_ok("screenshot_presented", after)
            gpu = client.result("query_gpu_world")
            assert gpu["enabled"] and gpu["complete"], gpu
            print("[gpu-fallbacks] PASS: zoomed engine view stays on GPU path")
            expect_visible_change("engine zoom", before, after, minimum=10)

            client.expect_ok("set_zoom_level", 0)
            client.expect_ok("camera_debug_center", city["x"], city["y"])
            client.expect_ok("screenshot_presented", before)
            client.expect_ok("debug_combat_flash", city["x"], city["y"])
            client.expect_ok("screenshot_presented", after)
            gpu = client.result("query_gpu_world")
            assert gpu["enabled"] and gpu["complete"], gpu
            print("[gpu-fallbacks] PASS: combat flash stays on GPU path")
            expect_visible_change("combat flash", before, after)
            return 0
    except (Ctp2Error, AssertionError, KeyError) as e:
        print(f"[gpu-fallbacks] FAIL: {e}")
        print(f"[gpu-fallbacks] see game log: {log}")
        return 1


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: gpu_world_fallbacks.py <path-to-ctp2>")
        sys.exit(2)
    sys.exit(run(sys.argv[1]))
