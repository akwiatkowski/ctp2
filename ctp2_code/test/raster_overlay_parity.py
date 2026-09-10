#!/usr/bin/env python3
"""GPU-rasterised overlays are bit-equal to the CPU composite (#15615).

gpu-raster-parity paints controlled terrain patches, which CLEARS rivers, and
runs fully lit — so the three overlay families the raster path gained (rivers,
fog, the grid) are not in its frames at all. This test covers exactly those:
the SAME natural map (rivers intact), captured under three states — lit,
fogged, and grid-on — with CTP2_GPU_RASTER off (reference) and on.

The bar is bit-equality, same as the terrain parity test, because nothing
about these overlays is approximate: rivers contain no shadow runs (measured:
667 copy / 1075 skip / 0 shadow across all 16 pieces), fog is applied at
decode time with the same 16-bit pixelutils_BlendFast the CPU uses — down to
the legacy quirk that a fogged river drops its last RLE row — and the grid is
two solid pixels per diamond row.
"""
import os
import struct
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "ctp2_code" / "test"))

from ctp2_client import Ctp2Client  # noqa: E402

CROP = 320
FIXTURE = "/tmp/ctp2-raster-overlay.sav"
RADIUS = 20


class NoRiverOnMap(RuntimeError):
    pass


def read_bmp_rgb(path):
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


def crop_pixels(path):
    w, h, px = read_bmp_rgb(path)
    cw, ch = min(CROP, w), min(CROP, h)
    left, top = (w - cw) // 2, (h - ch) // 2
    return [row[left:left + cw] for row in px[top:top + ch]]


def capture(binary, mode_on, out):
    env = os.environ.copy()
    env.update({"CTP2_GPU_LAYERS": "1", "CTP2_GPU_CAMERA": "1",
                "CTP2_GPU_QUADS": "1", "CTP2_GPU_WORLDMAP": "1",
                "CTP2_GPU_RASTER": "1" if mode_on else "0"})
    env.setdefault("CTP2_MODERN_SPRITES", "1")
    tag = "raster" if mode_on else "cpu"
    sock = f"/tmp/ctp2-rovp-{tag}-{os.getpid()}.sock"
    env["CTP2_SMOKE_SOCKET"] = sock

    shots = {}
    with Ctp2Client(binary, "ui", seed=42, players=4,
                    socket_path=sock, env=env,
                    log_path=str(out / f"{tag}.log")) as c:
        if not mode_on:
            # The UI build has no --seed: every map is random, and some maps
            # have no rivers anywhere (measured: debug_find_river returned
            # no_river_on_map on a real run). A scene without a river cannot
            # test river parity, so regenerate until the map has one.
            c.expect_ok("new_game")
            c.expect_ok("start_game")
            c.wait_game_loaded()
            probe = c.command("debug_find_river", 0, 0)
            if probe.get("detail") == "no_river_on_map":
                raise NoRiverOnMap("generated map has no river")
            assert probe.get("status") == "ok", probe
            c.expect_ok("save_game", FIXTURE)
        # Both renderers start from the same restored state and tile caches.
        c.expect_ok("load_game", FIXTURE)
        c.expect_ok("debug_deselect")
        c.expect_ok("set_show_city_names", 0)
        c.expect_ok("debug_worldmap_sprites", 0)

        armies = c.result("query_armies").get("armies", [])
        if not armies:
            raise RuntimeError("no army to centre on")
        pos = armies[0]["pos"]

        def settle_at(centre, name):
            path = out / f"{tag}-{name}.bmp"
            previous = None
            for _ in range(30):
                c.expect_ok("camera_debug_center", centre["x"], centre["y"])
                c.expect_ok("screenshot_presented", str(path))
                current = path.read_bytes()
                if previous == current:
                    return path
                previous = current
                time.sleep(0.2)
            raise RuntimeError(f"{tag}: frame never settled for {name}")

        def settle(name):
            return settle_at(pos, name)

        # LIT: natural map, rivers intact, everything visible.
        c.expect_ok("debug_reveal_patch", pos["x"], pos["y"], RADIUS)
        shots["lit"] = settle("lit")

        # GRID: the same scene with the tile grid composited into every cell.
        c.expect_ok("debug_set_grid", 1)
        shots["grid"] = settle("grid")
        c.expect_ok("debug_set_grid", 0)

        # FOGGED: explored-but-not-visible. debug_explore_patch adds a vision
        # reference and drops it, so the explored bit stays and the count is
        # back to zero everywhere a unit cannot actually see.
        #
        # Centre AWAY from the unit for this capture: around the unit its own
        # vision keeps a lit circle, and rivers inside it draw unfogged — the
        # first version of this test measured exactly that and could not see a
        # fogged river at all (removing the fogged-river last-row quirk passed).
        # An offset centre puts the whole crop in fog, rivers included.
        # ...and centre it on a KNOWN river cell, not a guessed offset. The
        # offset version of this capture also had no fogged river in the crop,
        # and the mutation slipped through a second time. Explore around the
        # river so the whole crop is fog.
        river = c.result("debug_find_river", pos["x"], pos["y"])["pos"]
        c.expect_ok("debug_explore_patch", pos["x"], pos["y"], RADIUS)
        c.expect_ok("debug_explore_patch", river["x"], river["y"], RADIUS)
        shots["fogged"] = settle_at(river, "fogged")
    return shots


def main():
    binary = str(Path(sys.argv[1]).resolve())
    out = Path("/tmp/ctp2-raster-overlay")
    out.mkdir(parents=True, exist_ok=True)
    if Path(FIXTURE).exists():
        Path(FIXTURE).unlink()

    # A new process returns to the main menu; new_game cannot be repeated
    # while the previous map is still open.
    for attempt in range(8):
        try:
            ref = capture(binary, False, out)
            break
        except NoRiverOnMap:
            if attempt == 7:
                raise
    test = capture(binary, True, out)

    failures = []
    for state in ("lit", "grid", "fogged"):
        a = crop_pixels(str(ref[state]))
        b = crop_pixels(str(test[state]))
        world = sum(1 for row in a for p in row if max(p) > 8)
        diff = sum(1 for ra, rb in zip(a, b)
                   for pa, pb in zip(ra, rb) if pa != pb)
        print(f"[raster-overlay] {state:7s} world_px={world:6d} "
              f"differing={diff}")
        # A scene with nothing in it would pass vacuously.
        if world < 20000:
            failures.append(f"{state}: only {world} world pixels in the crop — "
                            f"the scene does not exercise the overlay")
        if diff != 0:
            failures.append(f"{state}: {diff} pixels differ — the raster "
                            f"composite is not bit-equal to the CPU one")

    if failures:
        print("raster overlay parity failed:", file=sys.stderr)
        for f in failures:
            print(f"  {f}", file=sys.stderr)
        return 1
    print("[raster-overlay] rivers, grid and fog are bit-equal on the raster path")
    return 0


if __name__ == "__main__":
    sys.exit(main())
