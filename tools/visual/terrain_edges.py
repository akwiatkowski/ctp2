#!/usr/bin/env python3
"""Generate real-engine screenshots for terrain edge/transition review.

This is a visual debugging tool, not a pixel-perfect regression test. It edits a
small visible patch with controlled terrain pairs, screenshots the presented UI,
and writes a contact sheet so humans can judge whether GPU quad edges look
smooth enough.
"""

import argparse
import os
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


def capture_mode(binary, out_dir, mode_name, seed, players, pairs, patterns, radius):
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
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded()
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
    for mode in modes:
        all_shots.extend(capture_mode(binary, out_dir, mode, args.seed,
                                      args.players, args.pairs,
                                      args.patterns, args.radius))
    write_contact_sheet(out_dir, all_shots, args.crop_size, args.scale)


if __name__ == "__main__":
    main()
