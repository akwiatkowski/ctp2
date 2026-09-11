#!/usr/bin/env python3
"""The map must render on an ordinary game start with no render-forcing input.

Boots a game, sends no center/reveal/screenshot until the assert shot, then
requires non-trivial presented content. Catches the no-repaint disease where
the worldmap texture stays empty and only forced renders ever paint.
"""
import os
from pathlib import Path
import sys
import time

from PIL import Image

from ctp2_client import Ctp2Client

# Samples (8px grid) that must be non-black. Measured: units alone on an
# unbuilt map stay under ~40; a rendered spawn island is in the thousands.
MIN_CONTENT_SAMPLES = 60


def lit_samples(path, step=8, threshold=30):
    im = Image.open(path).convert("RGB")
    px = im.load()
    return sum(
        1
        for x in range(0, im.width, step)
        for y in range(0, im.height, step)
        if sum(px[x, y]) >= threshold
    )


def run():
    binary = Path(sys.argv[1] if len(sys.argv) > 1 else "build/ctp2").resolve()
    out = binary.parent / "worldmap-load-renders"
    out.mkdir(exist_ok=True)
    socket = f"/tmp/ctp2-load-renders-{os.getpid()}.sock"
    env = dict(os.environ, CTP2_SMOKE_SOCKET=socket, CTP2_GPU_LAYERS="1",
               CTP2_GPU_CAMERA="1", CTP2_GPU_QUADS="1", CTP2_GPU_WORLDMAP="1",
               CTP2_GPU_RASTER="1", CTP2_MODERN_SPRITES="1")
    with Ctp2Client(str(binary), "ui", env=env, socket_path=socket,
                    log_path=str(out / "game.log")) as client:
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded()
        # Ordinary start: no center, no reveal, no screenshot until the assert.
        time.sleep(5)
        gpu = client.result("query_gpu_world")
        assert gpu.get("worldmap_texture"), f"no worldmap texture: {gpu}"
        view = gpu.get("view_rect", [])
        assert len(view) == 4 and view[2] > view[0] and view[3] > view[1], (
            f"insane view rect: {view}"
        )
        shot = out / "spawn.bmp"
        client.expect_ok("screenshot_presented", str(shot))
        lit = lit_samples(shot)
        print(f"spawn lit samples: {lit} (origin={gpu.get('worldmap_origin')})")
        assert lit >= MIN_CONTENT_SAMPLES, (
            f"map did not render on start: {lit} lit samples in {shot}"
        )
        print(f"PASS worldmap-load-renders: {lit} lit samples")


if __name__ == "__main__":
    run()
