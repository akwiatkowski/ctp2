#!/usr/bin/env python3
"""
Pixel-proof harness for P11 GPU camera offset.

Runs ctp2 with GPU layers + camera, forces two known camera offsets via the
`camera_debug_set` debug command, captures the presented frame for each, and
reports whether the pixels actually shifted.

Usage:
    test/pan_pixel_proof.py <path-to-ctp2>

Exit codes:
    0  pixels shifted between offsets (camera offset works)
    1  pixels did not shift (camera offset is ignored / compositing bug)
    2  harness/runtime error
"""

import argparse
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402


def _bmp_pixel(data, x, y):
    """Return (r, g, b) at (x, y) from raw BMP bytes (24/32 bpp, bottom-up)."""
    import struct
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
    import struct
    return (struct.unpack_from("<i", data, 18)[0],
            abs(struct.unpack_from("<i", data, 22)[0]))


# The map viewport: skip the top menu bar (~30px) and the bottom control
# panel / minimap (~bottom third), so samples land on world-layer pixels
# that the camera transform is supposed to move.
def _map_region(w, h):
    return range(40, w - 40, 16), range(40, h * 3 // 5, 16)


def count_terrain(path):
    """Number of non-black sample points inside the map viewport region.

    A freshly loaded game shows unexplored (pure black) terrain; a camera
    shift of a uniformly black world is invisible, so the proof must first
    wait until real terrain pixels exist to move."""
    with open(path, "rb") as f:
        a = f.read()
    w, h = _bmp_dims(a)
    xs, ys = _map_region(w, h)
    return sum(1 for y in ys for x in xs if _bmp_pixel(a, x, y) != (0, 0, 0))


def diff_frames(path_a, path_b):
    with open(path_a, "rb") as f:
        a = f.read()
    with open(path_b, "rb") as f:
        b = f.read()

    aw, ah = _bmp_dims(a)
    bw, bh = _bmp_dims(b)
    assert (aw, ah) == (bw, bh), f"size mismatch: {aw}x{ah} vs {bw}x{bh}"

    # Sample the map viewport and count moved pixels among samples that are
    # terrain (non-black) in at least one frame — black-vs-black samples
    # (unexplored map) can never register a shift and would dilute the ratio.
    mismatches = 0
    samples = 0
    xs, ys = _map_region(aw, ah)
    for y in ys:
        for x in xs:
            pa, pb = _bmp_pixel(a, x, y), _bmp_pixel(b, x, y)
            if pa == (0, 0, 0) and pb == (0, 0, 0):
                continue
            samples += 1
            if pa != pb:
                mismatches += 1
    return samples, mismatches, (aw, ah)


def run_attempt(binary, env, socket_path, log, path0, path1):
    """One game launch. Returns an exit code, or None to re-roll the map."""
    with Ctp2Client(binary, "ui", socket_path=socket_path, log_path=log, env=env) as client:
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded()

        # The UI binary has no --seed, so the map is random. The view rect
        # lives in SKEWED iso tile space (tileX = x + y/2, per
        # maputils_MapX2TileX); centering near the horizontal wrap seam
        # produces a wrapped rect that draws black (legacy CenterMap quirk),
        # and a spawn near the top/bottom edge clamps the view off the
        # explored circle. Re-roll (fresh process) until comfortably interior.
        m = client.result("query_map")
        units = client.result("query_units")
        mine = [u for u in units["units"]
                if u["owner"] == units["visible_player"]]
        assert mine, "player has no units at game start"
        upos = mine[0]["pos"]
        tile_x = (upos["x"] + upos["y"] // 2) % m["width"]
        if not (8 <= tile_x <= m["width"] - 8
                and 20 <= upos["y"] <= m["height"] - 20):
            print(f"[pixel-proof] settler at ({upos['x']},{upos['y']}) "
                  f"(tileX={tile_x}) near seam/edge of "
                  f"{m['width']}x{m['height']} map — re-roll")
            return None

        # Center the view on the starting settler and force a terrain
        # redraw. Without it the viewport can sit over unexplored (pure
        # black) map — a camera shift of uniform black is invisible.
        # (build_city is NOT used: it pops the Build Manager modal, which
        # both covers the map and suppresses terrain drawing.)
        def has_terrain():
            # "modal" = a modal (Loading window) still suppresses terrain
            # drawing; anything else failing is a real error.
            r = client.command("camera_debug_center", upos["x"], upos["y"])
            if r.get("status") != "ok":
                if r.get("detail") == "modal":
                    return False
                raise Ctp2Error(f"camera_debug_center failed: {r}")
            client.expect_ok("camera_debug_set", 0, 0)
            client.expect_ok("screenshot_presented", path0)
            return count_terrain(path0) >= 20

        client.wait_until(has_terrain, timeout=60, desc="terrain visible")

        # Baseline (offset 0,0) is path0 from the wait above.
        # Forced offset (50, 50).
        client.expect_ok("camera_debug_set", 50, 50)
        client.expect_ok("screenshot_presented", path1)

        samples, mismatches, dims = diff_frames(path0, path1)
        print(f"[pixel-proof] frame dims: {dims[0]}x{dims[1]}")
        print(f"[pixel-proof] sampled {samples} points, {mismatches} mismatched")

        if mismatches <= samples * 0.05:
            print("[pixel-proof] FAIL: camera offset did NOT shift presented pixels")
            print("[pixel-proof] bug is likely in layer compositing, not input/cadence")
            return 1
        print("[pixel-proof] PASS: camera offset visibly shifted presented pixels")

        # ---- Phase 2: the glide. Stream pan-target pulses like a trackpad
        # (each within the recenter margin) and watch the presented frame pass
        # through SUB-TILE positions on its way — the actual "buttery" claim.
        # A tile-stepped pan only ever shows multiples of the 96px tile step.
        client.expect_ok("camera_debug_set", 0, 0)
        time.sleep(0.4)  # let the ease settle from the reset
        client.expect_ok("screenshot_presented", path0)

        shifts = []
        for i in range(10):
            client.expect_ok("camera_debug_pan", 20, 0)
            client.expect_ok("screenshot_presented", path1)
            shifts.append(measure_x_shift(path0, path1))
            time.sleep(0.10)
        # One more after the ease finishes.
        time.sleep(0.8)
        client.expect_ok("screenshot_presented", path1)
        shifts.append(measure_x_shift(path0, path1))

    print(f"[glide-proof] x-shifts vs baseline: {shifts}")
    total = shifts[-1]
    distinct = sorted(set(s for s in shifts if s is not None))
    subtile = [s for s in distinct
               if 4 < s < total - 4 and 4 < (s % 96) < 92]
    print(f"[glide-proof] total={total} distinct={distinct} subtile={subtile}")

    if total is None or total < 150:
        print("[glide-proof] FAIL: pan did not travel (expected ~200px)")
        return 1
    if len(subtile) < 2:
        print("[glide-proof] FAIL: no sub-tile intermediate positions — pan is tile-stepped")
        return 1
    print("[glide-proof] PASS: glide passed through sub-tile positions")
    return 0


def measure_x_shift(base_path, frame_path, max_shift=240):
    """Horizontal displacement of frame vs base (px), by 1-D correlation of
    terrain samples in the map region. Positive = content moved right."""
    with open(base_path, "rb") as f:
        a = f.read()
    with open(frame_path, "rb") as f:
        b = f.read()
    w, h = _bmp_dims(a)
    pts = [(x, y)
           for y in range(60, h * 3 // 5, 4)
           for x in range(max_shift + 4, w - max_shift - 4, 4)
           if _bmp_pixel(a, x, y) != (0, 0, 0)]
    if len(pts) < 50:
        return None
    pts = pts[::max(1, len(pts) // 400)]
    best = (0, 0)
    for d in range(-max_shift, max_shift + 1, 2):
        m = sum(1 for (x, y) in pts if _bmp_pixel(b, x + d, y) == _bmp_pixel(a, x, y))
        if m > best[0]:
            best = (m, d)
    return best[1]


def run(binary):
    env = os.environ.copy()
    env["CTP2_GPU_LAYERS"] = "1"
    env["CTP2_GPU_CAMERA"] = "1"
    run_id = os.getpid()
    socket_path = f"/tmp/ctp2-smoke-pan-{run_id}.sock"
    env["CTP2_SMOKE_SOCKET"] = socket_path
    log = f"/tmp/ctp2_pan_pixel_proof_{run_id}.log"
    print(f"[pixel-proof] {binary} (log -> {log})")

    path0 = f"/tmp/ctp2_pan_off0_{run_id}.bmp"
    path1 = f"/tmp/ctp2_pan_off50_{run_id}.bmp"
    for p in (path0, path1):
        try:
            os.unlink(p)
        except FileNotFoundError:
            pass

    try:
        for attempt in range(5):
            rc = run_attempt(binary, env, socket_path, log, path0, path1)
            if rc is not None:
                return rc
        print("[pixel-proof] no interior spawn in 5 rolls")
        return 2
    except (Ctp2Error, AssertionError) as e:
        print(f"[pixel-proof] harness error: {e}")
        print(f"[pixel-proof] see game log: {log}")
        return 2


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary", help="path to ctp2 UI binary")
    args = ap.parse_args()
    sys.exit(run(args.binary))


if __name__ == "__main__":
    sys.exit(main())
