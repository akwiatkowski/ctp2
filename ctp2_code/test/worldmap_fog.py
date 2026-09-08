#!/usr/bin/env python3
"""Fog of war composites into the whole-map tiles (#12839, P13 step 4).

Terrain on the whole-map path is composited once per cell and cached, so fog --
explored land that is not currently visible -- was simply missing from it: that
land drew at full brightness while the CPU and quad paths darkened it. It is
not a corner case; fog covers most of an explored map for most of a game. It is
also why worldmap-transition-parity has to reveal a radius around everything it
measures, so uncontrolled fog would not swamp the transitions under test.

Two checks, because either alone can pass while the feature is broken:

  1. Within the whole-map path, fog must actually darken the world: capture the
     same area fogged and then lit and require the brightness to move. A path
     that draws no fog changes nothing, which is exactly how this failed.
  2. The darkening must MATCH the reference path on the same map, so "draws
     something dark" cannot pass for "draws fog correctly".

Building the fogged state is not obvious. Vision::AddExplored is NOT "explore
without seeing" -- it is the same FillCircle(CIRCLE_OP_ADD) call as AddVisible,
because visibility is a reference count in the low bits with the explored flag
as the top bit. Fog is made by adding a visibility reference and then dropping
it: the count returns to zero while the explored bit stays set. That is what
debug_explore_patch does, and debug_vision_stats reports the resulting counts so
a failure can distinguish "fog was not drawn" from "fog was never created".
"""
import argparse
import json
import os
import struct
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "ctp2_code" / "test"))

from ctp2_client import Ctp2Client  # noqa: E402

MODE_ENV = {
    "gpu":      {"CTP2_GPU_QUADS": "1", "CTP2_GPU_WORLDMAP": "0"},
    "worldmap": {"CTP2_GPU_QUADS": "1", "CTP2_GPU_WORLDMAP": "1"},
}

CROP = 320
FIXTURE = None
RADIUS = 20


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


def crop_box(w, h):
    cw, ch = min(CROP, w), min(CROP, h)
    return (w - cw) // 2, (h - ch) // 2, cw, ch


def mean_brightness(path, state):
    """Mean brightness of the explored world in the centred crop."""
    w, h, px = read_bmp_rgb(path)
    left, top, cw, ch = crop_box(w, h)
    # The CPU window mirror subtracts the background-window margin at present;
    # quad/world-map geometry currently does not. Compare the SAME map patch,
    # not different terrain under identical screen rectangles. The full-frame
    # registration discrepancy remains visible in the acceptance gallery.
    if not state["worldmap"] and not state["complete"]:
        dx, dy = state["world_content_off"]
        left -= dx
        top -= dy
    assert 0 <= left and 0 <= top and left + cw <= w and top + ch <= h
    total = n = 0
    for y in range(top, top + ch):
        for x in range(left, left + cw):
            p = px[y][x]
            if max(p) <= 8:
                continue        # unexplored black is not fog
            total += sum(p)
            n += 1
    return (total / (3 * n)) if n else 0.0, n


def capture(mode, out, make_fixture, seed=42):
    env = os.environ.copy()
    env.update({"CTP2_GPU_LAYERS": "1", "CTP2_GPU_CAMERA": "1"})
    env.update(MODE_ENV[mode])
    env.setdefault("CTP2_MODERN_SPRITES", "1")
    sock = f"/tmp/ctp2-fog-{mode}-{os.getpid()}.sock"
    env["CTP2_SMOKE_SOCKET"] = sock

    with Ctp2Client(str(ROOT / "build" / "ctp2"), "ui", seed=seed, players=4,
                    socket_path=sock, env=env,
                    log_path=str(out / f"{mode}.log")) as c:
        if make_fixture:
            c.expect_ok("new_game")
            c.expect_ok("start_game")
            c.wait_game_loaded()
            c.expect_ok("save_game", FIXTURE)
            c.expect_ok("load_game", FIXTURE)
        else:
            c.expect_ok("load_game", FIXTURE)
        c.expect_ok("debug_deselect")
        c.expect_ok("set_show_city_names", 0)

        armies = c.result("query_armies").get("armies", [])
        if not armies:
            raise RuntimeError("no army to centre on")
        pos = armies[0]["pos"]

        states = {}

        def settle(name):
            path = out / f"{mode}-{name}.bmp"
            previous = None
            for _ in range(30):
                c.expect_ok("camera_debug_center", pos["x"], pos["y"])
                c.expect_ok("screenshot_presented", str(path))
                current = path.read_bytes()
                if previous == current:
                    states[name] = c.result("query_gpu_world")
                    return path
                previous = current
                time.sleep(0.2)
            raise RuntimeError(f"{mode}: frame never settled for {name}")

        c.expect_ok("debug_explore_patch", pos["x"], pos["y"], RADIUS)
        vision = c.result("debug_vision_stats", pos["x"], pos["y"], RADIUS)
        fogged = settle("fogged")

        c.expect_ok("debug_reveal_patch", pos["x"], pos["y"], RADIUS)
        lit = settle("lit")
        (out / f"{mode}-state.json").write_text(json.dumps(states, indent=2))
        return fogged, lit, vision, states


def main():
    global FIXTURE
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--fixture", type=Path, help="replay a retained base save")
    parser.add_argument("--out", type=Path, default=ROOT / "build" / "worldmap-fog" / time.strftime("%Y%m%d-%H%M%S"))
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    FIXTURE = str(args.fixture.resolve() if args.fixture else out / "base.sav")
    print(f"Fog evidence: {out}; replay with --fixture {FIXTURE}", flush=True)

    drops = {}
    failures = []
    for i, mode in enumerate(("gpu", "worldmap")):
        fogged, lit, vision, states = capture(mode, out, make_fixture=(i == 0 and not args.fixture), seed=args.seed)
        gpu = states["lit"]
        if vision["fogged"] < 100:
            failures.append(f"{mode}: only {vision['fogged']} cells ended up "
                            f"fogged — the scene never entered the state under "
                            f"test ({vision})")
            continue
        if mode == "worldmap" and not (gpu.get("worldmap")
                                       and gpu.get("worldmap_texture")):
            failures.append(f"not on the whole-map path: {gpu}")
            continue
        bf, nf = mean_brightness(fogged, states["fogged"])
        bl, nl = mean_brightness(lit, states["lit"])
        drops[mode] = 1.0 - (bf / bl) if bl else 0.0
        print(f"[fog] {mode:8s} fogged={bf:6.1f} lit={bl:6.1f} "
              f"darkening={drops[mode]:.3f} ({vision['fogged']} fogged cells)")

    if not failures:
        ref, test = drops["gpu"], drops["worldmap"]
        # The failure this exists for: the whole-map path drew fogged land at
        # full brightness, so the two states were byte-identical.
        if test < 0.10:
            failures.append(f"the whole-map path barely darkens under fog "
                            f"({test:.3f}) — it is not compositing fog")
        elif abs(test - ref) > 0.12:
            failures.append(f"the whole-map path darkens by {test:.3f} against "
                            f"the quad path's {ref:.3f} — fog strength differs")

    if failures:
        print("worldmap fog check failed:", file=sys.stderr)
        for f in failures:
            print(f"  {f}", file=sys.stderr)
        return 1
    print("[fog] whole-map fog matches the quad path")
    return 0


if __name__ == "__main__":
    sys.exit(main())
