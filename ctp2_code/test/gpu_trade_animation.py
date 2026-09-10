#!/usr/bin/env python3
"""A real resource trade route must animate and clear in whole-map mode."""
import argparse
import json
import os
from pathlib import Path
import time
from PIL import Image, ImageChops
from ctp2_client import Ctp2Client


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', nargs='?', type=Path, default=Path('build/ctp2_render'))
    binary = parser.parse_args().binary.resolve()
    out = binary.parent / 'gpu-trade-animation'
    out.mkdir(exist_ok=True)
    socket = f'/tmp/ctp2-trade-{os.getpid()}.sock'
    env = dict(os.environ, CTP2_SMOKE_SOCKET=socket, CTP2_GPU_LAYERS='1',
               CTP2_GPU_CAMERA='1', CTP2_GPU_QUADS='1', CTP2_GPU_WORLDMAP='1',
               CTP2_GPU_RASTER='1', CTP2_MODERN_SPRITES='1')
    with Ctp2Client(str(binary), 'ui', seed=42, env=env, socket_path=socket,
                    log_path=str(out / 'game.log')) as c:
        c.expect_ok('new_game')
        c.expect_ok('start_game')
        c.wait_game_loaded()
        grass = next(t for t in c.result('query_terrains')['terrains']
                     if t['internal'] == 'TERRAIN_GRASSLAND')['id']
        for y in range(15, 22):
            for x in range(15, 25):
                c.expect_ok('debug_set_terrain', x, y, grass)
        c.expect_ok('debug_set_good', 17, 18, 1)
        c.expect_ok('debug_gallery_case', 'city', 17, 18)
        c.expect_ok('debug_gallery_case', 'city', 22, 18)
        c.expect_ok('debug_reveal_patch', 20, 18, 20)
        # This five-tile grass corridor costs two transport points.
        for _ in range(2):
            c.expect_ok('create_unit', 'UNIT_CARAVAN', 17, 18)
        c.expect_ok('establish_trade_route', 0, 1)
        assert len(c.result('query_trade_routes')['routes']) == 1
        time.sleep(0.5)  # allow queued city dialogs to open before closing them
        c.expect_ok('debug_close_build_manager')
        c.expect_ok('debug_deselect')
        c.expect_ok('set_show_city_names', 0)
        c.expect_ok('camera_debug_center', 20, 18)
        initial = c.result('query_gpu_world')
        frames = []
        counts = []
        for index, enabled in enumerate((0, 1, 1, 0)):
            c.expect_ok('debug_trade_animation', enabled)
            time.sleep(0.5)
            path = out / f'{index}-trade-{enabled}.bmp'
            c.expect_ok('screenshot_presented', path)
            frames.append(Image.open(path).convert('RGB').crop((300, 40, 1000, 565)))
            state = c.result('query_gpu_world')
            assert state['camera'] == initial['camera']
            assert state['worldmap'] and state['worldmap_texture']
            assert not state['sprite_fallback_reason'], state
            counts.append(state['sprite_quads'])
        assert counts[1] == counts[0] + 1 and counts[2] == counts[1], counts
        assert counts[3] == counts[0], counts
        for index in (1, 2):
            pixels = ImageChops.difference(frames[0], frames[index]).tobytes()
            changed = sum(max(pixels[i:i+3]) > 8 for i in range(0, len(pixels), 3))
            assert changed > 20, f'trade actor submitted but invisible: {changed} pixels'
        assert ImageChops.difference(frames[1], frames[2]).getbbox(), 'trade actor did not move'
        assert not ImageChops.difference(frames[0], frames[3]).getbbox(), 'trade animation left stale pixels'
        c.expect_ok('save_game', out / 'scene.sav')
        (out / 'state.json').write_text(json.dumps(state, indent=2))
    print('PASS: real trade actor appears, moves and disappears without camera refresh', flush=True)


if __name__ == '__main__':
    run()
