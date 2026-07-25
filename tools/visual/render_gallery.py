#!/usr/bin/env python3
"""Generate CPU-left/GPU-right screenshots for human renderer review."""

import argparse
import json
import os
import re
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "ctp2_code" / "test"))

from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402


PATTERNS = {
    "vertical": lambda dx, dy: dx >= 0,
    "horizontal": lambda dx, dy: dy >= 0,
    "diagonal": lambda dx, dy: dx >= dy,
    "checker": lambda dx, dy: ((dx + dy) & 1) == 0,
}


def slug(s):
    return re.sub(r"[^a-z0-9]+", "-", s.lower()).strip("-")


def visible_center(client):
    armies = client.result("query_armies").get("armies", [])
    if armies:
        return armies[0]["pos"]
    cities = client.result("query_cities").get("cities", [])
    if cities:
        return cities[0]["pos"]
    world = client.result("query_world")
    return {"x": world["width"] // 2, "y": world["height"] // 2}


def terrain_lookup(client):
    terrains = client.result("query_terrains")["terrains"]
    lookup = {}
    for terrain in terrains:
        name = terrain.get("internal") or terrain.get("name") or str(terrain["id"])
        clean = slug(name.replace("TERRAIN_", ""))
        lookup[clean] = terrain
    return lookup


def paint_patch(client, center, terrain_a, terrain_b, pattern, radius):
    client.expect_ok("camera_debug_center", center["x"], center["y"])
    choose_b = PATTERNS[pattern]
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            terrain = terrain_b if choose_b(dx, dy) else terrain_a
            r = client.command("debug_set_terrain", center["x"] + dx, center["y"] + dy, terrain["id"])
            if r.get("status") != "ok" and r.get("detail") != "out_of_bounds":
                raise RuntimeError(f"debug_set_terrain failed: {r}")
    client.expect_ok("debug_clear_terrain_layers", center["x"], center["y"], 60)


def set_zoom(client, zoom):
    if zoom < 0:
        return
    client.expect_ok("set_zoom_level", zoom)


def zoom_label(zoom):
    return "default" if zoom < 0 else str(zoom)


def make_base_fixture(binary, out_dir, seed, players):
    env = os.environ.copy()
    env["CTP2_GPU_LAYERS"] = "1"
    env["CTP2_GPU_CAMERA"] = "1"
    env["CTP2_GPU_QUADS"] = "1"
    env.setdefault("CTP2_MODERN_SPRITES", "1")
    socket = f"/tmp/ctp2-gallery-base-{os.getpid()}.sock"
    env["CTP2_SMOKE_SOCKET"] = socket
    fixture = out_dir / "base-fixture.json"
    with Ctp2Client(str(binary), "ui", seed=seed, players=players,
                    socket_path=socket, env=env,
                    log_path=str(out_dir / "base.log")) as client:
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded()
        center = visible_center(client)
        client.expect_ok("debug_clear_terrain_layers", center["x"], center["y"], 60)
        client.expect_ok("save_game", fixture)
    return fixture


def open_client(binary, out_dir, mode, fixture):
    env = os.environ.copy()
    env["CTP2_GPU_LAYERS"] = "1"
    env["CTP2_GPU_CAMERA"] = "1"
    env["CTP2_GPU_QUADS"] = "1" if mode == "gpu" else "0"
    env.setdefault("CTP2_MODERN_SPRITES", "1")
    socket = f"/tmp/ctp2-gallery-{mode}-{os.getpid()}.sock"
    env["CTP2_SMOKE_SOCKET"] = socket
    client = Ctp2Client(str(binary), "ui", socket_path=socket, env=env,
                        log_path=str(out_dir / f"{mode}.log"))
    client.expect_ok("load_game", fixture)
    client.expect_ok("debug_deselect")
    return client


def center_crop(img, width, height):
    left = max(0, (img.width - width) // 2)
    top = max(0, (img.height - height) // 2)
    return img.crop((left, top, left + width, top + height))


def pixel_accuracy(cpu, gpu):
    if cpu.size != gpu.size:
        raise ValueError(f"pixel_accuracy needs equal sizes, got {cpu.size} and {gpu.size}")
    total = cpu.width * cpu.height
    if total == 0:
        return 0.0
    cpu_bytes = cpu.tobytes()
    gpu_bytes = gpu.tobytes()
    matches = sum(1 for i in range(0, len(cpu_bytes), 3) if cpu_bytes[i:i + 3] == gpu_bytes[i:i + 3])
    return matches * 100.0 / total


def compose_pair(cpu_bmp, gpu_bmp, out_png, title):
    from PIL import Image, ImageDraw, ImageFont

    cpu = Image.open(cpu_bmp).convert("RGB")
    gpu = Image.open(gpu_bmp).convert("RGB")
    crop_w = min(cpu.width, gpu.width)
    crop_h = min(cpu.height, gpu.height)
    cpu = center_crop(cpu, crop_w, crop_h)
    gpu = center_crop(gpu, crop_w, crop_h)
    accuracy = pixel_accuracy(cpu, gpu)
    label_h = 34
    font = ImageFont.load_default()
    w = cpu.width + gpu.width
    h = max(cpu.height, gpu.height) + label_h
    img = Image.new("RGB", (w, h), (24, 24, 24))
    draw = ImageDraw.Draw(img)
    draw.text((8, 8), f"CPU  {title}", fill=(235, 235, 235), font=font)
    draw.text((cpu.width + 8, 8), f"GPU  pixel accuracy {accuracy:.4f}%", fill=(235, 235, 235), font=font)
    img.paste(cpu, (0, label_h))
    img.paste(gpu, (cpu.width, label_h))
    img.save(out_png)
    return {
        "pixel_accuracy": accuracy,
        "compared_width": crop_w,
        "compared_height": crop_h,
        "compared_pixels": crop_w * crop_h,
    }


def capture_case(client, out_dir, local_index, total, index, name, mode, fixture, apply_case):
    client.expect_ok("load_game", fixture)
    client.expect_ok("debug_deselect")
    client.expect_ok("set_show_city_names", 0)
    client.expect_ok("debug_scenario_start_flags", 0)
    center = visible_center(client)
    ok = apply_case(client, center)
    if ok is False:
        return {"case": name, "skipped": True, "reason": "fixture command failed"}
    r = client.command("camera_debug_center", center["x"], center["y"])
    if r.get("status") != "ok" and r.get("detail") == "modal":
        return {"case": name, "skipped": True, "reason": "modal"}
    if r.get("status") != "ok":
        raise Ctp2Error(f"camera_debug_center failed: {r}")
    base = f"{index:05d}-{slug(name)}"
    tmp_dir = out_dir / "tmp"
    tmp_dir.mkdir(exist_ok=True)
    bmp = tmp_dir / f"{base}-{mode}.bmp"
    print(f"[{mode}] {local_index}/{total} temporary capture -> {bmp}", flush=True)
    screenshot_command = "screenshot_map_only" if name.startswith("scene ") else "screenshot_presented"
    client.expect_ok(screenshot_command, bmp)
    record = {"case": name, "bmp": bmp}
    if mode == "gpu":
        record["gpu"] = client.result("query_gpu_world")
    return record


def convert_bmp_to_png(bmp_path, png_dir):
    from PIL import Image

    png_dir.mkdir(exist_ok=True)
    png_path = png_dir / bmp_path.with_suffix(".png").name
    Image.open(bmp_path).convert("RGB").save(png_path)
    bmp_path.unlink()
    return png_path


def latest_run_dir(base_dir):
    runs = [p for p in base_dir.iterdir() if p.is_dir()] if base_dir.exists() else []
    return sorted(runs)[-1] if runs else None


def completed_indices(out_dir):
    done = set()
    for path in out_dir.glob("*.png"):
        m = re.match(r"(\d+)-", path.name)
        if m:
            done.add(int(m.group(1)))
    return done


def load_accuracy_records(path):
    if not path.exists():
        return []
    with path.open() as f:
        data = json.load(f)
    if isinstance(data, dict):
        return data.get("results", [])
    return data


def capture_mode(binary, out_dir, mode, fixture, cases, limit):
    client = open_client(binary, out_dir, mode, fixture)
    records = []
    try:
        total = len(cases) if not limit else min(len(cases), limit)
        for local_index, (index, name, apply_case) in enumerate(cases, start=1):
            if limit and local_index > limit:
                break
            remaining = total - local_index
            print(f"[{mode}] {local_index}/{total} #{index:05d} rendering: {name} ({remaining} left)", flush=True)
            records.append(capture_case(client, out_dir, local_index, total, index, name, mode, fixture, apply_case))
    finally:
        client.close()
    return records


def terrain_cases(terrains, zooms, patterns, radius):
    items = list(terrains.items())
    for zoom in zooms:
        zlabel = zoom_label(zoom)
        for i, (name_a, terrain_a) in enumerate(items):
            for name_b, terrain_b in items[i + 1:]:
                for pattern in patterns:
                    name = f"terrain z{zlabel} {name_a} vs {name_b} {pattern}"

                    def apply(client, center, ta=terrain_a, tb=terrain_b, pat=pattern, z=zoom):
                        set_zoom(client, z)
                        paint_patch(client, center, ta, tb, pat, radius)

                    yield name, apply


def overlay_cases(zooms):
    for zoom in zooms:
        zlabel = zoom_label(zoom)
        yield f"overlay z{zlabel} city names", lambda c, center, z=zoom: (set_zoom(c, z), c.expect_ok("set_show_city_names", 1))
        yield f"overlay z{zlabel} scenario flags", lambda c, center, z=zoom: (set_zoom(c, z), c.expect_ok("debug_scenario_start_flags", 1))
        yield f"overlay z{zlabel} terrain overlay", lambda c, center, z=zoom: terrain_overlay(c, center, z)
        yield f"effect z{zlabel} combat flash", lambda c, center, z=zoom: combat_flash(c, center, z)
        yield f"city z{zlabel} walls", lambda c, center, z=zoom: city_defense(c, center, z, "walls")
        yield f"city z{zlabel} forcefield", lambda c, center, z=zoom: city_defense(c, center, z, "forcefield")


def gallery_cases(zooms):
    units = [
        "UNIT_SUBMARINE",
        "UNIT_NUCLEAR_SUBMARINE",
        "UNIT_STEALTH_BOMBER",
        "UNIT_SPACE_PLANE",
        "UNIT_CYBER_NINJA",
        "UNIT_CLERIC",
        "UNIT_INFECTOR",
    ]
    for zoom in zooms:
        zlabel = zoom_label(zoom)
        yield f"gallery z{zlabel} combat flash", lambda c, center, z=zoom: gallery_case(c, center, z, "combat_flash")
        yield f"gallery z{zlabel} terrain overlay", lambda c, center, z=zoom: gallery_case(c, center, z, "terrain_overlay")
        for unit in units:
            yield f"gallery z{zlabel} unit {unit.lower()}", lambda c, center, u=unit, z=zoom: gallery_case(c, center, z, "unit", u)


def preferred_terrain(terrains, names, fallback_index):
    for name in names:
        for key, terrain in terrains.items():
            if name in key:
                return terrain
    return list(terrains.values())[fallback_index]


def scene_cases(terrains, zooms):
    terrain_a = preferred_terrain(terrains, ["grass", "plains"], 0)
    terrain_b = preferred_terrain(terrains, ["desert", "sand", "hill"], min(1, len(terrains) - 1))
    units = ["UNIT_MARINE", "UNIT_STEALTH_BOMBER", "UNIT_SETTLER"]
    for zoom in zooms:
        zlabel = zoom_label(zoom)
        for unit in units:
            yield f"scene z{zlabel} 5x5 real tiles real {unit.lower()}", lambda c, center, u=unit, z=zoom: real_scene(c, center, z, terrain_a, terrain_b, u)


def city_pos(client):
    cities = client.result("query_cities").get("cities", [])
    if cities:
        return cities[0]["pos"], cities[0]["index"]
    pos = client.result("build_city")["pos"]
    cities = client.result("query_cities")["cities"]
    return pos, next(c["index"] for c in cities if c["pos"] == pos)


def terrain_overlay(client, center, zoom):
    set_zoom(client, zoom)
    client.expect_ok("debug_terrain_overlay", center["x"] + 1, center["y"])


def combat_flash(client, center, zoom):
    set_zoom(client, zoom)
    client.expect_ok("debug_combat_flash", center["x"] + 1, center["y"])


def city_defense(client, center, zoom, kind):
    set_zoom(client, zoom)
    pos, idx = city_pos(client)
    client.expect_ok("debug_city_defense", idx, kind)
    center["x"] = pos["x"]
    center["y"] = pos["y"]


def gallery_case(client, center, zoom, kind, arg=None):
    set_zoom(client, zoom)
    target = dict(center)
    if kind.startswith("city") or kind == "underwater_city":
        world = client.result("query_world")
        target["x"] += 10
        target["y"] += 10
        target["x"] = max(0, min(world["width"] - 1, target["x"]))
        target["y"] = max(0, min(world["height"] - 1, target["y"]))
        center["x"] = target["x"]
        center["y"] = target["y"]
    command_args = [kind, target["x"], target["y"]]
    if arg:
        command_args.append(arg)
    r = client.command("debug_gallery_case", *command_args)
    if r.get("status") != "ok":
        print(f"SKIP {kind} {arg or ''}: {r.get('detail')}")
        return False
    return True


def real_scene(client, center, zoom, terrain_a, terrain_b, unit):
    set_zoom(client, zoom)
    paint_patch(client, center, terrain_a, terrain_b, "checker", 2)
    return gallery_case(client, center, zoom, "unit", unit)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", default=str(ROOT / "build" / "ctp2"))
    parser.add_argument("--render-binary", default=str(ROOT / "build" / "ctp2_render"),
                        help="binary used when scene cases need render-tool-only commands")
    parser.add_argument("--out", default=str(ROOT / "build" / "render-gallery"))
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--players", type=int, default=4)
    parser.add_argument("--radius", type=int, default=4)
    parser.add_argument("--zooms", nargs="*", type=int, default=[-1],
                        help="-1 keeps the game's default zoom")
    parser.add_argument("--patterns", nargs="*", default=list(PATTERNS), choices=list(PATTERNS))
    parser.add_argument("--families", nargs="*", default=["terrain", "overlay", "gallery"],
                        choices=["terrain", "overlay", "gallery", "scene"])
    parser.add_argument("--limit", type=int, default=0, help="0 means no limit")
    parser.add_argument("--continue", "--continue-run", dest="continue_run", action="store_true",
                        help="reuse the latest output directory and render the next missing cases")
    args = parser.parse_args()

    out_base = Path(args.out)
    out_dir = latest_run_dir(out_base) if args.continue_run else None
    if out_dir is None:
        out_dir = out_base / time.strftime("%Y%m%d-%H%M%S")
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f"output: {out_dir}", flush=True)
    binary = Path(args.render_binary if "scene" in args.families else args.binary)
    fixture = out_dir / "base-fixture.json"
    if not fixture.exists():
        fixture = make_base_fixture(binary, out_dir, args.seed, args.players)
    manifest = out_dir / "manifest.jsonl"
    accuracy_path = out_dir / "pixel-accuracy.json"

    probe = open_client(binary, out_dir, "gpu", fixture)
    try:
        terrains = terrain_lookup(probe)
    finally:
        probe.close()

    cases = []
    if "terrain" in args.families:
        cases.extend(terrain_cases(terrains, args.zooms, args.patterns, args.radius))
    if "overlay" in args.families:
        cases.extend(overlay_cases(args.zooms))
    if "gallery" in args.families:
        cases.extend(gallery_cases(args.zooms))
    if "scene" in args.families:
        cases.extend(scene_cases(terrains, args.zooms))
    indexed_cases = [(index, name, apply_case) for index, (name, apply_case) in enumerate(cases, start=1)]
    if args.continue_run:
        done = completed_indices(out_dir)
        indexed_cases = [case for case in indexed_cases if case[0] not in done]
        print(f"continue: {len(done)} completed, {len(indexed_cases)} remaining", flush=True)
    if args.limit:
        indexed_cases = indexed_cases[:args.limit]
    print(f"planned: {len(indexed_cases)} cases", flush=True)

    cpu_records = capture_mode(binary, out_dir, "cpu", fixture, indexed_cases, 0)
    gpu_records = capture_mode(binary, out_dir, "gpu", fixture, indexed_cases, 0)
    accuracy_records = load_accuracy_records(accuracy_path) if args.continue_run else []
    accuracy_by_file = {record["file"]: record for record in accuracy_records}
    with manifest.open("a" if args.continue_run else "w") as f:
        for cpu_record, gpu_record in zip(cpu_records, gpu_records):
            if cpu_record.get("skipped") or gpu_record.get("skipped"):
                record = {
                    "case": cpu_record["case"],
                    "skipped": True,
                    "reason": cpu_record.get("reason") or gpu_record.get("reason"),
                }
                f.write(json.dumps(record, sort_keys=True) + "\n")
                continue
            index = int(Path(cpu_record["bmp"]).name.split("-", 1)[0])
            base = f"{index:05d}-{slug(cpu_record['case'])}"
            out_png = out_dir / f"{base}.png"
            print(f"[compose] #{index:05d} {cpu_record['case']} -> {out_png}", flush=True)
            source_dir = out_dir / "sources"
            cpu_png = convert_bmp_to_png(cpu_record["bmp"], source_dir)
            gpu_png = convert_bmp_to_png(gpu_record["bmp"], source_dir)
            accuracy = compose_pair(cpu_png, gpu_png, out_png, cpu_record["case"])
            accuracy_record = {
                "file": out_png.name,
                "case": cpu_record["case"],
                "pixel_accuracy": round(accuracy["pixel_accuracy"], 6),
                "compared_width": accuracy["compared_width"],
                "compared_height": accuracy["compared_height"],
                "compared_pixels": accuracy["compared_pixels"],
            }
            accuracy_by_file[out_png.name] = accuracy_record
            record = {"file": out_png.name, "case": cpu_record["case"], "gpu": gpu_record.get("gpu", {}), **accuracy_record}
            f.write(json.dumps(record, sort_keys=True) + "\n")
            print(f"{record['file']} pixel_accuracy={accuracy_record['pixel_accuracy']:.4f}%")
    accuracy_records = [accuracy_by_file[name] for name in sorted(accuracy_by_file)]
    with accuracy_path.open("w") as f:
        json.dump({"results": accuracy_records}, f, indent=2, sort_keys=True)
    count = len(cpu_records)
    print(f"wrote {count} review images to {out_dir}")
    print(f"wrote pixel accuracy to {accuracy_path}")


if __name__ == "__main__":
    main()
