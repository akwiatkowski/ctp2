#!/usr/bin/env python3
"""National borders composite into the whole-map tiles, in BOTH styles (#14260).

CTP2 draws political borders two completely different ways depending on a
graphics option: "smooth" stamps a corner icon per edge, and the line style
draws a coloured edge pixel by pixel through DrawColoredBorderEdge. The
whole-map path (P13) composited only the icon style -- the line style derived
its own view-relative position and clipped against a full-screen surface, so
into a tile-sized scratch it drew nothing at all. A player with smooth borders
turned off simply had no borders on that path.

The check is the one the grid fix used, because it cannot pass vacuously:
toggle the overlay and require the presented pixels to actually change. A style
that composites nothing changes zero pixels no matter how healthy the flags
look.
"""
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "ctp2_code" / "test"))

from ctp2_client import Ctp2Client  # noqa: E402

# Borders are thin: a handful of tile edges, a few pixels wide. The grid fix
# measured 190 changed pixels and that was a whole-map overlay, so the bar here
# is deliberately low -- it separates "drew something" from "drew nothing",
# which is the actual failure mode.
MIN_CHANGED = 40


def read_bmp_rgb(path):
    import struct
    data = Path(path).read_bytes()
    off = struct.unpack_from("<I", data, 10)[0]
    w = struct.unpack_from("<i", data, 18)[0]
    h_raw = struct.unpack_from("<i", data, 22)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]
    h = abs(h_raw)
    row_size = ((bpp * w + 31) // 32) * 4
    px = []
    for y in range(h):
        src_y = h - 1 - y if h_raw > 0 else y
        row = []
        for x in range(w):
            p = off + src_y * row_size + x * (bpp // 8)
            row.append((data[p + 2], data[p + 1], data[p]))
        px.append(row)
    return w, h, px


def changed_pixels(a, b):
    wa, ha, pa = read_bmp_rgb(a)
    wb, hb, pb = read_bmp_rgb(b)
    if (wa, ha) != (wb, hb):
        raise RuntimeError(f"frame size mismatch {wa}x{ha} vs {wb}x{hb}")
    n = 0
    for y in range(ha):
        ra, rb = pa[y], pb[y]
        for x in range(wa):
            if ra[x] != rb[x]:
                n += 1
    return n


def main():
    binary = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "build" / "ctp2")
    out = Path("/tmp/ctp2-worldmap-borders")
    out.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env.update({"CTP2_GPU_LAYERS": "1", "CTP2_GPU_CAMERA": "1",
                "CTP2_GPU_QUADS": "1", "CTP2_GPU_WORLDMAP": "1"})
    env.setdefault("CTP2_MODERN_SPRITES", "1")
    sock = f"/tmp/ctp2-borders-{os.getpid()}.sock"
    env["CTP2_SMOKE_SOCKET"] = sock

    failures = []
    with Ctp2Client(str(binary), "ui", seed=42, players=4, socket_path=sock,
                    env=env, log_path="/tmp/ctp2-worldmap-borders.log") as c:
        c.expect_ok("new_game")
        c.expect_ok("start_game")
        c.wait_game_loaded()
        # Borders only exist around owned territory, so found a city first.
        c.expect_ok("build_city")
        c.expect_ok("debug_deselect")
        c.expect_ok("set_show_city_names", 0)

        cities = c.result("query_cities").get("cities", [])
        if not cities:
            print("no city was founded — nothing owns territory", file=sys.stderr)
            return 1
        pos = cities[0]["pos"]
        c.expect_ok("camera_debug_center", pos["x"], pos["y"])
        c.expect_ok("debug_reveal_patch", pos["x"], pos["y"], 20)

        gpu = c.result("query_gpu_world")
        if not (gpu.get("worldmap") and gpu.get("worldmap_texture")):
            print(f"not on the whole-map path: {gpu}", file=sys.stderr)
            return 1

        def capture(name):
            p = out / f"{name}.bmp"
            c.expect_ok("camera_debug_center", pos["x"], pos["y"])
            c.expect_ok("screenshot_presented", str(p))
            return p

        # These settings persist to userprofile.txt when the game exits, so
        # leaving them changed would alter the user's game and skew every later
        # test run. Captured before the first change, restored after the last.
        original = None
        for style, smooth in (("smooth", 1), ("line", 0)):
            r = c.result("debug_set_borders", 0, smooth)
            if original is None:
                original = r.get("was", {})
            off = capture(f"{style}-off")
            c.expect_ok("debug_set_borders", 1, smooth)
            on = capture(f"{style}-on")
            n = changed_pixels(off, on)
            print(f"[borders] {style:6s} style: {n} pixels changed when toggled on")
            if n < MIN_CHANGED:
                failures.append(
                    f"{style} borders changed only {n} pixels (< {MIN_CHANGED}) "
                    f"— the whole-map tiles are not compositing this style")

        if original:
            c.expect_ok("debug_set_borders",
                        original.get("borders", 1), original.get("smooth", 1))
            print(f"[borders] restored borders={original.get('borders')} "
                  f"smooth={original.get('smooth')}")

    if failures:
        print("worldmap border check failed:", file=sys.stderr)
        for f in failures:
            print(f"  {f}", file=sys.stderr)
        return 1
    print("[borders] both styles composite into the whole-map tiles")
    return 0


if __name__ == "__main__":
    sys.exit(main())
