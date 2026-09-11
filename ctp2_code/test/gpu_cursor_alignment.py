#!/usr/bin/env python3
"""Check live mouse picking and the presented tile outline at pan/zoom offsets."""
import os
from pathlib import Path
import sys
import time

from PIL import Image, ImageChops
from ctp2_client import Ctp2Client


def run():
    binary = Path(sys.argv[1] if len(sys.argv) > 1 else "build/ctp2_render").resolve()
    out = binary.parent / "gpu-cursor-alignment"
    out.mkdir(exist_ok=True)
    socket = f"/tmp/ctp2-cursor-{os.getpid()}.sock"
    env = dict(os.environ, CTP2_SMOKE_SOCKET=socket, CTP2_GPU_LAYERS="1",
               CTP2_GPU_CAMERA="1", CTP2_GPU_QUADS="1", CTP2_GPU_WORLDMAP="1",
               CTP2_GPU_RASTER="1", CTP2_MODERN_SPRITES="1")
    with Ctp2Client(str(binary), "ui", env=env, socket_path=socket,
                    log_path=str(out / "game.log")) as client:
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded()
        client.expect_ok("debug_deselect")
        client.expect_ok("set_show_city_names", 0)
        client.expect_ok("debug_reveal_patch", 30, 30, 14)
        client.expect_ok("debug_clear_terrain_layers", 30, 30, 14)
        client.expect_ok("set_zoom_level", 5)
        client.expect_ok("camera_debug_center", 30, 30)
        shot = out / "viewport.bmp"
        client.expect_ok("screenshot_presented", shot)
        width, height = Image.open(shot).size
        sx, sy = width // 2, height // 2
        for zoom, dx, dy in ((1, 0, 0), (1, 96, 72), (1.5, -20, -40), (0.9, 0, 0)):
            client.expect_ok("camera_debug_zoom", zoom)
            client.expect_ok("camera_debug_set", dx, dy)
            images = []
            for visible in (0, 1):
                result = client.expect_ok("debug_mouse_tile", sx, sy, visible)["detail"]
                if zoom == 1 and dx == 0 and dy == 0:
                    assert result == "hit=1 tile=30,30", result
                time.sleep(0.2)
                shot = out / f"cursor-{zoom}-{dx}-{dy}-{visible}.bmp"
                client.expect_ok("screenshot_presented", shot)
                images.append(Image.open(shot).convert("RGB"))
            # Restrict to the map: UI animation is unrelated to the tile outline.
            crop = (sx - 200, sy - 150, sx + 200, sy + 150)
            box = ImageChops.difference(*[im.crop(crop) for im in images]).getbbox()
            assert box, "tile cursor was not presented"
            left, top, right, bottom = box
            assert left <= 200 < right and top <= 150 < bottom, (
                f"cursor misses mouse at zoom={zoom}, pan={dx},{dy}: {box}"
            )
            print(f"PASS zoom={zoom}, pan={dx},{dy}: {result}, outline={box}")


if __name__ == "__main__":
    run()
