#!/usr/bin/env python3
"""Generate real-engine screenshots for terrain edge/transition review.

This is a visual debugging tool, not a pixel-perfect regression test. It edits a
small visible patch with controlled terrain pairs, screenshots the presented UI,
and writes a contact sheet so humans can judge whether GPU quad edges look
smooth enough.
"""

import argparse
import os
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "ctp2_code" / "test"))

from ctp2_client import Ctp2Client  # noqa: E402


DEFAULT_PAIRS = [
    "grassland:plains",
    "grassland:forest",
    "grassland:hill",
    "plains:desert",
    "forest:hill",
]

PATTERNS = {
    "vertical": lambda dx, dy: dx >= 0,
    "horizontal": lambda dx, dy: dy >= 0,
    "diagonal": lambda dx, dy: dx >= dy,
    "checker": lambda dx, dy: ((dx + dy) & 1) == 0,
}


def terrain_lookup(client):
    terrains = client.result("query_terrains")["terrains"]
    lookup = {}
    for terrain in terrains:
        keys = [terrain.get("name", ""), terrain.get("internal", "")]
        for key in keys:
            clean = key.lower().replace("terrain_", "").replace(" ", "_")
            if clean:
                lookup[clean] = terrain
                lookup[clean.replace("_", "")] = terrain
    return lookup


def resolve_pair(lookup, spec):
    left, right = spec.split(":", 1)
    a = lookup.get(left.lower().replace(" ", "_"))
    b = lookup.get(right.lower().replace(" ", "_"))
    if not a or not b:
        known = ", ".join(sorted({t["internal"] for t in lookup.values()})[:20])
        raise SystemExit(f"unknown terrain pair {spec!r}; examples: {known}")
    return a, b


def visible_center(client):
    armies = client.result("query_armies").get("armies", [])
    if armies:
        return armies[0]["pos"]
    cities = client.result("query_cities").get("cities", [])
    if cities:
        return cities[0]["pos"]
    world = client.result("query_world")
    return {"x": world["width"] // 2, "y": world["height"] // 2}


def paint_patch(client, center, terrain_a, terrain_b, pattern, radius):
    cx, cy = center["x"], center["y"]
    choose_b = PATTERNS[pattern]
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            terrain = terrain_b if choose_b(dx, dy) else terrain_a
            r = client.command("debug_set_terrain", cx + dx, cy + dy, terrain["id"])
            # Near map edges a few cells can fall outside the world. Keep the
            # fixture robust; the visible center normally avoids this.
            if r.get("status") != "ok" and r.get("detail") != "out_of_bounds":
                raise RuntimeError(f"debug_set_terrain failed: {r}")
    client.expect_ok("debug_clear_terrain_layers", cx, cy, 60)


def capture_mode(binary, out_dir, mode_name, seed, players, pairs, patterns, radius,
                 save_fixture=None, load_fixture=None):
    env = os.environ.copy()
    env["CTP2_GPU_LAYERS"] = "1"
    env["CTP2_GPU_CAMERA"] = "1"
    env["CTP2_GPU_QUADS"] = "1" if mode_name == "gpu" else "0"
    env.setdefault("CTP2_MODERN_SPRITES", "1")

    socket = f"/tmp/ctp2-terrain-edges-{mode_name}-{os.getpid()}.sock"
    env["CTP2_SMOKE_SOCKET"] = socket
    log = out_dir / f"{mode_name}.log"
    shots = []
    with Ctp2Client(str(binary), "ui", seed=seed, players=players,
                    socket_path=socket, env=env, log_path=str(log)) as client:
        if load_fixture:
            client.expect_ok("load_game", load_fixture)
        else:
            client.expect_ok("new_game")
            client.expect_ok("start_game")
            client.wait_game_loaded()
            if save_fixture:
                client.expect_ok("save_game", save_fixture)
        client.expect_ok("debug_deselect")
        client.expect_ok("set_show_city_names", 0)

        lookup = terrain_lookup(client)
        center = visible_center(client)
        client.expect_ok("camera_debug_center", center["x"], center["y"])

        for pair_spec in pairs:
            terrain_a, terrain_b = resolve_pair(lookup, pair_spec)
            for pattern in patterns:
                paint_patch(client, center, terrain_a, terrain_b, pattern, radius)
                client.expect_ok("camera_debug_center", center["x"], center["y"])
                path = out_dir / f"{mode_name}-{pair_spec.replace(':', '-')}-{pattern}.bmp"
                client.expect_ok("screenshot_presented", path)
                gpu = client.result("query_gpu_world")
                shots.append({
                    "path": path,
                    "mode": mode_name,
                    "pair": pair_spec,
                    "pattern": pattern,
                    "gpu": gpu,
                })
    return shots


def center_crop(img, size):
    if size <= 0:
        return img
    w, h = img.size
    crop_w = min(size, w)
    crop_h = min(size, h)
    left = (w - crop_w) // 2
    top = (h - crop_h) // 2
    return img.crop((left, top, left + crop_w, top + crop_h))


def write_contact_sheet(out_dir, shots, crop_size, scale):
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        print("Pillow not installed; BMP screenshots were still written.")
        return

    thumb_w = crop_size * scale
    label_h = 36
    pad = 12
    font = ImageFont.load_default()
    images = []
    for shot in shots:
        img = center_crop(Image.open(shot["path"]).convert("RGB"), crop_size)
        thumb = img.resize((img.width * scale, img.height * scale), Image.Resampling.NEAREST)
        crop_path = shot["path"].with_suffix(".crop.png")
        thumb.save(crop_path)
        shot["crop_path"] = crop_path
        images.append((shot, thumb))

    cols = 2
    cell_w = thumb_w + pad
    cell_h = max(img.height for _, img in images) + label_h + pad
    rows = (len(images) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell_w + pad, rows * cell_h + pad), (28, 28, 28))
    draw = ImageDraw.Draw(sheet)

    for i, (shot, img) in enumerate(images):
        x = pad + (i % cols) * cell_w
        y = pad + (i // cols) * cell_h
        label = f"{shot['mode']}  {shot['pair']}  {shot['pattern']}"
        if shot["mode"] == "gpu":
            label += f"  complete={shot['gpu'].get('complete')}"
        draw.text((x, y), label, fill=(235, 235, 235), font=font)
        sheet.paste(img, (x, y + label_h))

    path = out_dir / "terrain-edge-contact-sheet.png"
    sheet.save(path)
    print(f"wrote {path}")


def read_bmp_rgb(path):
    data = Path(path).read_bytes()
    off = struct.unpack_from("<I", data, 10)[0]
    w = struct.unpack_from("<i", data, 18)[0]
    h_raw = struct.unpack_from("<i", data, 22)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]
    if bpp not in (24, 32):
        raise ValueError(f"{path}: unsupported BMP bpp {bpp}")
    h = abs(h_raw)
    row_size = ((bpp * w + 31) // 32) * 4
    pixels = []
    for y in range(h):
        src_y = h - 1 - y if h_raw > 0 else y
        row = []
        for x in range(w):
            p = off + src_y * row_size + x * (bpp // 8)
            b, g, r = data[p], data[p + 1], data[p + 2]
            row.append((r, g, b))
        pixels.append(row)
    return w, h, pixels


def terrain_profile(path, crop_size):
    w, h, pixels = read_bmp_rgb(path)
    crop_w = min(crop_size, w) if crop_size > 0 else w
    crop_h = min(crop_size, h) if crop_size > 0 else h
    left = (w - crop_w) // 2
    top = (h - crop_h) // 2

    world = 0
    green = 0
    yellow = 0
    dark = 0
    for y in range(top, top + crop_h):
        for x in range(left, left + crop_w):
            r, g, blue = pixels[y][x]
            if r <= 8 and g <= 8 and blue <= 8:
                continue
            world += 1
            if g >= r and g >= blue:
                green += 1
            if r >= g and r >= blue and r > 80:
                yellow += 1
            if max(r, g, blue) < 50:
                dark += 1

    return {
        "world_pixels": world,
        "green_ratio": green / world if world else 0.0,
        "yellow_ratio": yellow / world if world else 0.0,
        "dark_ratio": dark / world if world else 0.0,
    }


def check_gpu_vs_cpu(shots, crop_size, min_world_coverage_ratio, max_color_ratio_delta):
    by_key = {(s["mode"], s["pair"], s["pattern"]): s for s in shots}
    failures = []
    for shot in shots:
        if shot["mode"] != "gpu":
            continue
        cpu = by_key.get(("cpu", shot["pair"], shot["pattern"]))
        if not cpu:
            failures.append(f"missing CPU reference for {shot['pair']} {shot['pattern']}")
            continue
        if not shot["gpu"].get("enabled") or not shot["gpu"].get("complete"):
            failures.append(f"GPU frame incomplete for {shot['pair']} {shot['pattern']}: {shot['gpu']}")
            continue
        gpu_profile = terrain_profile(shot["path"], crop_size)
        cpu_profile = terrain_profile(cpu["path"], crop_size)
        coverage_ratio = (
            gpu_profile["world_pixels"] / cpu_profile["world_pixels"]
            if cpu_profile["world_pixels"] else 0.0
        )
        color_delta = max(
            abs(gpu_profile["green_ratio"] - cpu_profile["green_ratio"]),
            abs(gpu_profile["yellow_ratio"] - cpu_profile["yellow_ratio"]),
            abs(gpu_profile["dark_ratio"] - cpu_profile["dark_ratio"]),
        )
        print(
            f"{shot['pair']} {shot['pattern']}: "
            f"coverage={coverage_ratio:.3f} "
            f"color_delta={color_delta:.3f} "
            f"gpu_world={gpu_profile['world_pixels']} "
            f"cpu_world={cpu_profile['world_pixels']}"
        )
        if coverage_ratio < min_world_coverage_ratio or color_delta > max_color_ratio_delta:
            failures.append(
                f"{shot['pair']} {shot['pattern']} exceeds threshold: "
                f"coverage={coverage_ratio:.3f}/{min_world_coverage_ratio}, "
                f"color_delta={color_delta:.3f}/{max_color_ratio_delta}"
            )
    if failures:
        raise SystemExit("terrain GPU/CPU check failed:\n" + "\n".join(failures))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", default=str(ROOT / "build" / "ctp2"))
    parser.add_argument("--out", default="/tmp/ctp2-terrain-edges")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--players", type=int, default=4)
    parser.add_argument("--radius", type=int, default=4)
    parser.add_argument("--pairs", nargs="*", default=DEFAULT_PAIRS,
                        help="terrain pairs like grassland:plains")
    parser.add_argument("--patterns", nargs="*", default=list(PATTERNS),
                        choices=list(PATTERNS))
    parser.add_argument("--mode", choices=["both", "gpu", "cpu"], default="both")
    parser.add_argument("--check", action="store_true",
                        help="fail if GPU and CPU crops differ beyond thresholds")
    parser.add_argument("--min-world-coverage-ratio", type=float, default=0.45)
    parser.add_argument("--max-color-ratio-delta", type=float, default=0.15)
    parser.add_argument("--crop-size", type=int, default=420,
                        help="center crop size before contact-sheet scaling")
    parser.add_argument("--scale", type=int, default=2,
                        help="nearest-neighbor upscale for inspecting tile edges")
    args = parser.parse_args()

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    binary = Path(args.binary)

    modes = ["gpu", "cpu"] if args.mode == "both" else [args.mode]
    all_shots = []
    fixture = out_dir / "terrain-edge-fixture.json"
    for mode in modes:
        save_fixture = str(fixture) if args.check and mode == "gpu" else None
        load_fixture = str(fixture) if args.check and mode == "cpu" and fixture.exists() else None
        all_shots.extend(capture_mode(binary, out_dir, mode, args.seed,
                                      args.players, args.pairs,
                                      args.patterns, args.radius,
                                      save_fixture=save_fixture,
                                      load_fixture=load_fixture))
    write_contact_sheet(out_dir, all_shots, args.crop_size, args.scale)
    if args.check:
        if args.mode != "both":
            raise SystemExit("--check requires --mode both")
        check_gpu_vs_cpu(all_shots, args.crop_size,
                         args.min_world_coverage_ratio,
                         args.max_color_ratio_delta)


if __name__ == "__main__":
    main()
