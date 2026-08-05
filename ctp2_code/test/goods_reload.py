#!/usr/bin/env python3
"""Goods survive a save/load round trip (#14230).

TileInfo::m_goodActor is a UI sprite pointer, so it is deliberately not
serialised. Nothing recreated the actors after a load, which meant every
resource on the map was invisible after loading any save -- on every render
path, in a released-looking build, with no error anywhere. A new game showed
them only because map generation happens to post-process every tile.

The check is the goods probe in query_gpu_world: `cells` is how many visible
cells carry a good, `emitted` how many actually produced a sprite to draw and
`no_actor` how many had no actor to draw with. A correct load reports
emitted == cells and no_actor == 0, the same as a new game.

Runs both render paths. The whole-map path (P13) composites goods into the
map texture and the quad path draws them per frame, so a fix in one does not
imply the other.
"""
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "ctp2_code" / "test"))

from ctp2_client import Ctp2Client  # noqa: E402

MODE_ENV = {
    "gpu":      {"CTP2_GPU_QUADS": "1", "CTP2_GPU_WORLDMAP": "0"},
    "worldmap": {"CTP2_GPU_QUADS": "1", "CTP2_GPU_WORLDMAP": "1"},
}


def visible_center(client):
    armies = client.result("query_armies").get("armies", [])
    if armies:
        return armies[0]["pos"]
    world = client.result("query_world")
    return {"x": world["width"] // 2, "y": world["height"] // 2}


def probe(binary, mode, fixture, make_fixture):
    env = os.environ.copy()
    env["CTP2_GPU_LAYERS"] = "1"
    env["CTP2_GPU_CAMERA"] = "1"
    env.update(MODE_ENV[mode])
    env.setdefault("CTP2_MODERN_SPRITES", "1")
    tag = f"{mode}-{'new' if make_fixture else 'loaded'}"
    sock = f"/tmp/ctp2-goods-{tag}-{os.getpid()}.sock"
    env["CTP2_SMOKE_SOCKET"] = sock

    with Ctp2Client(str(binary), "ui", seed=42, players=4, socket_path=sock,
                    env=env, log_path=f"/tmp/ctp2-goods-{tag}.log") as c:
        if make_fixture:
            c.expect_ok("new_game")
            c.expect_ok("start_game")
            c.wait_game_loaded()
            c.expect_ok("save_game", fixture)
        else:
            c.expect_ok("load_game", fixture)
        c.expect_ok("debug_deselect")

        center = visible_center(c)
        c.expect_ok("camera_debug_center", center["x"], center["y"])
        c.expect_ok("debug_reveal_patch", center["x"], center["y"], 60)
        # Centre once more so a full frame is composited before reading the
        # probe: it reports what the last build emitted, not what it would.
        c.expect_ok("camera_debug_center", center["x"], center["y"])
        return c.result("query_gpu_world").get("goods", {})


def main():
    binary = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "build" / "ctp2")
    fixture = "/tmp/ctp2-goods-reload.sav"

    failures = []
    for mode in ("gpu", "worldmap"):
        fresh = probe(binary, mode, fixture, make_fixture=True)
        print(f"[goods] {mode:8s} new game: {fresh}")
        # A map with no goods in view would pass everything vacuously.
        if fresh.get("cells", 0) <= 0:
            failures.append(f"{mode}: new game has no goods in view — "
                            f"nothing to test ({fresh})")
            continue

        loaded = probe(binary, mode, fixture, make_fixture=False)
        print(f"[goods] {mode:8s} loaded:   {loaded}")
        if loaded.get("no_actor", -1) != 0:
            failures.append(f"{mode}: {loaded.get('no_actor')} cells carry a good "
                            f"but have no actor after load ({loaded})")
        if loaded.get("emitted") != loaded.get("cells"):
            failures.append(f"{mode}: emitted {loaded.get('emitted')} of "
                            f"{loaded.get('cells')} goods after load ({loaded})")

    if failures:
        print("goods reload check failed:", file=sys.stderr)
        for f in failures:
            print(f"  {f}", file=sys.stderr)
        return 1
    print("[goods] every good kept its actor across save/load on both paths")
    return 0


if __name__ == "__main__":
    sys.exit(main())
