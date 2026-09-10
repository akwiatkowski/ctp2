#!/usr/bin/env python3
"""Sample normal actor painting on macOS in matched CPU and GPU scenes."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'ctp2_code/test'))
from ctp2_client import Ctp2Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build/ctp2')
    parser.add_argument('--seconds', type=int, default=10)
    parser.add_argument('--out', type=Path, default=ROOT / 'build/sprite-profile')
    args = parser.parse_args()
    out = args.out.resolve() / time.strftime('%Y%m%d-%H%M%S')
    out.mkdir(parents=True)
    for mode in ('cpu', 'gpu'):
        enabled = str(int(mode == 'gpu'))
        env = dict(os.environ, CTP2_GPU_LAYERS='1', CTP2_GPU_CAMERA='1',
                   CTP2_GPU_QUADS=enabled, CTP2_GPU_WORLDMAP=enabled,
                   CTP2_GPU_RASTER=enabled, CTP2_MODERN_SPRITES='1')
        socket = f'/tmp/ctp2-profile-{os.getpid()}-{mode}.sock'
        env['CTP2_SMOKE_SOCKET'] = socket
        print(f'{mode}: preparing 40-actor scene', flush=True)
        with Ctp2Client(str(args.binary.resolve()), 'ui', seed=42, env=env,
                        socket_path=socket, log_path=str(out / f'{mode}.log')) as client:
            client.expect_ok('new_game')
            client.expect_ok('start_game')
            client.wait_game_loaded()
            center = client.result('query_armies')['armies'][0]['pos']
            terrain = next(t for t in client.result('query_terrains')['terrains']
                           if t['internal'] == 'TERRAIN_GRASSLAND')['id']
            client.expect_ok('debug_reveal_patch', center['x'], center['y'], 20)
            world = client.result('query_world')
            for dy in range(2, 7):
                for dx in range(2, 10):
                    x = (center['x'] + dx) % world['width']
                    y = min(world['height'] - 1, center['y'] + dy)
                    client.expect_ok('debug_set_terrain', x, y, terrain)
                    client.expect_ok('create_unit', 'UNIT_ARCHER', x, y)
            client.expect_ok('camera_debug_center', center['x'] + 5,
                             min(world['height'] - 1, center['y'] + 4))
            time.sleep(1)
            client.expect_ok('screenshot_presented', out / f'{mode}.bmp')
            (out / f'{mode}-state.json').write_text(json.dumps(client.result('query_gpu_world'), indent=2))
            # Apple's sampler measures running and sleeping stacks. Inspect the
            # main-thread tree; totals are samples, not GPU execution timings.
            subprocess.run(['sample', str(client.proc.pid), str(args.seconds),
                            '-file', str(out / f'{mode}.sample.txt')], check=True)
    print(out, flush=True)


if __name__ == '__main__':
    main()
