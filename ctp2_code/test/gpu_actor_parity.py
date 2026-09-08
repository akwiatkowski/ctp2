#!/usr/bin/env python3
"""Compare action pixels and registration, including missing-atlas fallback."""
import argparse
import json
import os
import shutil
from pathlib import Path
import sys
import time
from PIL import Image, ImageChops, ImageFilter
from ctp2_client import Ctp2Client

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/visual'))
from render_gallery import make_base_fixture, visible_center, safe_patch_center


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', nargs='?', type=Path, default=ROOT / 'build/ctp2_render')
    args = parser.parse_args()
    binary = args.binary.resolve()
    out = binary.parent / 'gpu-actor-parity'
    out.mkdir(exist_ok=True)
    fixture = make_base_fixture(binary, out, 42, 4)
    frames = {}
    picks = {}
    for mode in ('cpu', 'gpu', 'fallback'):
        enabled = str(int(mode != 'cpu'))
        socket = f'/tmp/ctp2-actor-parity-{os.getpid()}-{mode}.sock'
        env = dict(os.environ, CTP2_SMOKE_SOCKET=socket, CTP2_GPU_LAYERS='1',
                   CTP2_GPU_CAMERA='1', CTP2_GPU_QUADS=enabled,
                   CTP2_GPU_WORLDMAP=enabled, CTP2_GPU_RASTER=enabled,
                   CTP2_MODERN_SPRITES='1')
        if mode == 'fallback':
            # A real missing-assets installation must keep GPU mode enabled.
            # MODERN_SPRITES=0 also disables quads and would miss this case.
            home = out / 'missing-atlas-home'
            home.mkdir(exist_ok=True)
            source = Path(os.environ.get('CTP2_HOME', str(Path.home() / '.ctp2')))
            if not (home / 'original_data').exists():
                (home / 'original_data').symlink_to((source / 'original_data').resolve(), target_is_directory=True)
            shutil.copyfile(source / 'userprofile.txt', home / 'userprofile.txt')
            env['CTP2_HOME'] = str(home)
        with Ctp2Client(str(binary), 'ui', env=env, socket_path=socket,
                        log_path=str(out / f'{mode}.log')) as c:
            c.expect_ok('load_game', fixture)
            c.wait_game_loaded()
            c.expect_ok('debug_deselect')
            c.expect_ok('set_show_city_names', 0)
            center = safe_patch_center(c, visible_center(c), 3)
            x, y = center['x'], center['y']
            terrain = next(t for t in c.result('query_terrains')['terrains']
                           if t['internal'] == 'TERRAIN_GRASSLAND')['id']
            c.expect_ok('debug_set_terrain', x, y, terrain)
            c.expect_ok('create_unit', 'UNIT_ARCHER', x, y)
            c.expect_ok('debug_reveal_patch', x, y, 14)
            for zoom in (0, 4, 5):
                c.expect_ok('set_zoom_level', zoom)
                c.expect_ok('camera_debug_center', x, y)
                for action in (1, 3):
                    for facing in (0, 5):
                        key = f'z{zoom}-action{action}-facing{facing}'
                        pair = []
                        for opacity in (0, 15):
                            c.expect_ok('debug_sprite_pose', x, y, action, -2, facing, opacity)
                            time.sleep(0.2)
                            path = out / f'{mode}-{key}-alpha{opacity}.bmp'
                            c.expect_ok('screenshot_presented', path)
                            pair.append(Image.open(path).convert('RGB'))
                        frames[mode, key] = pair
            c.expect_ok('set_zoom_level', 5)
            c.expect_ok('camera_debug_center', x, y)
            for scale, dx, dy in ((1.0, 30, -30), (1.5, -20, -40), (0.9, 0, 0)):
                c.expect_ok('camera_debug_zoom', scale)
                c.expect_ok('camera_debug_set', dx, dy)
                pair = []
                key = f'camera-{scale}-{dx}-{dy}'
                for opacity in (0, 15):
                    c.expect_ok('debug_sprite_pose', x, y, 1, -2, 5, opacity)
                    time.sleep(0.2)
                    camera = c.result('query_gpu_world')['camera']
                    assert abs(camera['zoom'] - scale) < 0.001, camera
                    assert abs(camera['off_x'] - dx) < 0.001 and abs(camera['off_y'] - dy) < 0.001, camera
                    path = out / f'{mode}-{key}-alpha{opacity}.bmp'
                    c.expect_ok('screenshot_presented', path)
                    pair.append(Image.open(path).convert('RGB'))
                frames[mode, key] = pair
                picks[mode, key] = [c.command('pick_tile', sx, sy)['detail']
                                    for sx, sy in ((450, 400), (600, 450), (700, 500))]
            c.expect_ok('camera_debug_zoom', 1)
            c.expect_ok('camera_debug_set', 0, 0)
            # Isolate a resource sprite from its unchanged terrain, in both
            # explored-fog and visible states. No unit/city is near this patch.
            gx, gy = 35, 25
            c.expect_ok('debug_set_terrain', gx, gy, terrain)
            c.expect_ok('set_zoom_level', 5)
            c.expect_ok('camera_debug_center', gx, gy)
            for fog in (True, False):
                pair = []
                key = f'good-{"fog" if fog else "lit"}'
                for present in (0, 1):
                    c.expect_ok('debug_set_good', gx, gy, present)
                    # Explore first: revealing adds persistent visibility
                    # references, so a later explore call cannot make it fogged.
                    c.expect_ok('debug_explore_patch' if fog else 'debug_reveal_patch', gx, gy, 14)
                    time.sleep(0.2)
                    vision = c.result('debug_vision_stats', gx, gy, 0)
                    assert vision['fogged'] == int(fog), vision
                    path = out / f'{mode}-{key}-present{present}.bmp'
                    c.expect_ok('screenshot_presented', path)
                    pair.append(Image.open(path).convert('RGB'))
                frames[mode, key] = pair
            state = c.result('query_gpu_world')
            (out / f'{mode}-state.json').write_text(json.dumps(state, indent=2))
            if mode in ('gpu', 'fallback'):
                assert state['worldmap'] and state['worldmap_texture'], state
            if mode == 'gpu':
                assert not state['sprite_fallback_reason'], state
            if mode == 'fallback':
                assert state['sprite_fallback_reason'], 'fixture did not exercise missing atlases'
        print(f'{mode}: captured action pairs', flush=True)
    for (mode, key), tiles in picks.items():
        assert tiles == picks['cpu', key], f'{mode} {key}: picking disagrees: {tiles} != {picks["cpu", key]}'
    results = []
    for (mode, key), (blank, actor) in frames.items():
        if mode != 'cpu':
            continue
        reference = ImageChops.difference(blank, actor)
        # Only pixels changed by this actor's opacity, away from the HUD.
        mask = [(x, y) for y in range(350, 560) for x in range(400, 760)
                if max(reference.getpixel((x, y))) > 8]
        assert len(mask) > 20, f'{key}: empty reference actor mask'
        for other in ('gpu', 'fallback'):
            before, after = frames[other, key]
            diff = ImageChops.difference(before, after)
            # CPU scaled SPR sampling and GPU texture sampling differ at edges
            # by one pixel. Restrict to the centred actor (other armies animate),
            # then permit that rasterization difference, never a camera offset.
            diff = diff.filter(ImageFilter.MaxFilter(3))
            hit = sum(max(diff.getpixel(p)) > 8 for p in mask) / len(mask)
            assert hit > 0.9, f'{other} {key}: actor missing or misregistered ({hit:.3f})'
            if key.startswith('good-'):
                error = sum(abs(a - b) for pos in mask
                            for a, b in zip(actor.getpixel(pos), after.getpixel(pos))) / (3 * len(mask))
                assert error < 8, f'{other} {key}: resource colour/fog mismatch (mean error {error:.2f})'
            results.append({'mode': other, 'case': key, 'reference_pixels': len(mask), 'visible_fraction': hit})
    (out / 'results.json').write_text(json.dumps(results, indent=2))
    print(f'PASS: {len(results)} action/scale/facing comparisons, including CPU fallback', flush=True)


if __name__ == '__main__':
    run()
