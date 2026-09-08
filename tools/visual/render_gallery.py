#!/usr/bin/env python3
"""Generate CPU-left/GPU-right screenshots for human renderer review."""

import argparse
import html
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


def human_player(client):
    for player in client.result("query_players").get("players", []):
        if player.get("human"):
            return player
    raise RuntimeError("no human player")


def validate_human_alive(client, context):
    player = human_player(client)
    if player.get("dead") or player.get("victory_label") == "defeat":
        raise RuntimeError(f"{context}: human player is defeated")
    if player.get("num_units", 0) <= 0 and player.get("num_armies", 0) <= 0 and player.get("num_cities", 0) <= 0:
        raise RuntimeError(f"{context}: human player has no units, armies, or cities")


def reveal_patch(client, center, radius):
    client.expect_ok("debug_reveal_patch", center["x"], center["y"], radius)


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


def safe_patch_center(client, center, radius):
    world = client.result("query_world")
    margin = radius + 2
    offset = radius * 2 + 6
    x = center["x"] + offset
    y = center["y"] + offset
    if x >= world["width"] - margin:
        x = center["x"] - offset
    if y >= world["height"] - margin:
        y = center["y"] - offset
    return {
        "x": max(margin, min(world["width"] - margin - 1, x)),
        "y": max(margin, min(world["height"] - margin - 1, y)),
    }


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
        client.expect_ok("debug_set_good_richness", 50)
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded()
        center = visible_center(client)
        client.expect_ok("debug_clear_terrain_layers", center["x"], center["y"], 60)
        client.expect_ok("save_game", fixture)
    return fixture


def open_client(binary, out_dir, mode, fixture, legacy_reference=False):
    env = os.environ.copy()
    env["CTP2_GPU_LAYERS"] = "1"
    env["CTP2_GPU_CAMERA"] = "1"
    for flag in ("CTP2_GPU_QUADS", "CTP2_GPU_WORLDMAP", "CTP2_GPU_RASTER"):
        env[flag] = "1" if mode == "gpu" else "0"
    env["CTP2_MODERN_SPRITES"] = "0" if mode == "cpu" and legacy_reference else "1"
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
    draw.text((cpu.width + 8, 8), f"GPU  full-frame matching pixels {accuracy:.4f}%", fill=(235, 235, 235), font=font)
    img.paste(cpu, (0, label_h))
    img.paste(gpu, (cpu.width, label_h))
    img.save(out_png)
    return {
        "pixel_accuracy": accuracy,
        "compared_width": crop_w,
        "compared_height": crop_h,
        "compared_pixels": crop_w * crop_h,
    }


def capture_case(client, out_dir, local_index, total, index, name, mode, fixture, apply_case, reveal_radius):
    reuse = name.startswith("sprite ") and " archer " in name
    if not reuse or not getattr(client, "gallery_pose_scene", False):
        client.expect_ok("load_game", fixture)
        client.gallery_base_center = visible_center(client)
        client.gallery_pose_scene = reuse
        client.gallery_pose_actor = False
    client.expect_ok("debug_deselect")
    client.expect_ok("set_show_city_names", 0)
    client.expect_ok("debug_scenario_start_flags", 0)
    validate_human_alive(client, f"{name} before setup")
    center = dict(client.gallery_base_center)
    ok = apply_case(client, center)
    if ok is False:
        return {"case": name, "skipped": True, "reason": "fixture command failed"}
    if " fogged" in name:
        client.expect_ok("debug_explore_patch", center["x"], center["y"], reveal_radius)
    else:
        reveal_patch(client, center, reveal_radius)
    validate_human_alive(client, f"{name} after setup")
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
    coverage = client.result("debug_worldmap_build", "rebuild") if mode == "gpu" else None
    # Give the normal 75 ms actor tick a frame after changing camera/pose.
    time.sleep(0.15)
    if "combat flash" in name or name.endswith(" effect"):
        client.expect_ok("debug_combat_flash", center["x"], center["y"])
        time.sleep(0.1)
    r = client.command("screenshot_presented", bmp)
    if r.get("status") != "ok":
        raise Ctp2Error(f"screenshot failed: {r}")
    validate_human_alive(client, f"{name} after screenshot")
    record = {"case": name, "bmp": bmp}
    if mode == "gpu":
        record["gpu"] = client.result("query_gpu_world")
        record["coverage"] = coverage
        assert record["gpu"]["worldmap_texture"], record["gpu"]
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


def capture_mode(binary, out_dir, mode, fixture, cases, limit, reveal_radius, legacy_reference=False):
    client = open_client(binary, out_dir, mode, fixture, legacy_reference)
    records = []
    try:
        total = len(cases) if not limit else min(len(cases), limit)
        for local_index, (index, name, apply_case) in enumerate(cases, start=1):
            if limit and local_index > limit:
                break
            remaining = total - local_index
            print(f"[{mode}] {local_index}/{total} #{index:05d} rendering: {name} ({remaining} left)", flush=True)
            records.append(capture_case(client, out_dir, local_index, total, index, name, mode, fixture, apply_case, reveal_radius))
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
                        patch_center = safe_patch_center(client, center, radius)
                        center.update(patch_center)
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


def sprite_cases(zooms):
    # Fixed action/frame/facing removes timing differences between CPU/GPU captures.
    for zoom in zooms:
        for action, label in enumerate(("move", "attack", "idle", "victory", "work")):
            for facing in (0, 4, 5, 7):
                for frame in (0, -2, -1):
                    name = f"sprite z{zoom_label(zoom)} archer {label} facing{facing} frame{frame}"
                    def apply(c, center, z=zoom, a=action, f=frame, d=facing):
                        set_zoom(c, z)
                        # Place the actor away from the initial stack and city labels.
                        center.update(safe_patch_center(c, center, 3))
                        grass = next(t for t in c.result("query_terrains")["terrains"] if t.get("internal") == "TERRAIN_GRASSLAND")
                        c.expect_ok("debug_set_terrain", center["x"], center["y"], grass["id"])
                        if not c.gallery_pose_actor:
                            c.expect_ok("create_unit", "UNIT_ARCHER", center["x"], center["y"])
                            c.gallery_pose_actor = True
                        result = c.command("debug_sprite_pose", center["x"], center["y"], a, f, d)
                        if result.get("detail") == "unsupported_pose":
                            return False
                        if result.get("status") != "ok":
                            raise Ctp2Error(str(result))
                    yield name, apply
        for label, opacity, fog in (("transparent", 8, 0), ("fogged", 15, 1)):
            def shaded(c, center, z=zoom, opacity=opacity, fog=fog):
                set_zoom(c, z)
                center.update(safe_patch_center(c, center, 3))
                grass = next(t for t in c.result("query_terrains")["terrains"] if t.get("internal") == "TERRAIN_GRASSLAND")
                c.expect_ok("debug_set_terrain", center["x"], center["y"], grass["id"])
                if not c.gallery_pose_actor:
                    c.expect_ok("create_unit", "UNIT_ARCHER", center["x"], center["y"])
                    c.gallery_pose_actor = True
                c.expect_ok("debug_sprite_pose", center["x"], center["y"], 0, 0, 5, opacity, fog)
            yield f"sprite z{zoom_label(zoom)} archer {label}", shaded
        for state in ("lit", "fogged"):
            def good(c, center, z=zoom):
                set_zoom(c, z)
                center.update(c.result("debug_find_good", center["x"], center["y"])["pos"])
            yield f"good z{zoom_label(zoom)} {state}", good
        for kind in ("city", "city_walls", "city_forcefield", "underwater_city"):
            yield f"sprite z{zoom_label(zoom)} {kind}", lambda c, center, z=zoom, k=kind: gallery_case(c, center, z, k)
        yield f"sprite z{zoom_label(zoom)} effect", lambda c, center, z=zoom: combat_flash(c, center, z)


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
    if kind.startswith("city"):
        grass = next(t for t in client.result("query_terrains")["terrains"] if t.get("internal") == "TERRAIN_GRASSLAND")
        client.expect_ok("debug_set_terrain", target["x"], target["y"], grass["id"])
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
    parser.add_argument("--legacy-reference", action="store_true", help="use legacy SPR software drawing for the CPU reference")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--players", type=int, default=4)
    parser.add_argument("--radius", type=int, default=4)
    parser.add_argument("--reveal-radius", type=int, default=14,
                        help="visible/explored radius forced around each rendered fixture")
    parser.add_argument("--zooms", nargs="*", type=int, default=[-1],
                        help="-1 keeps the game's default zoom")
    parser.add_argument("--patterns", nargs="*", default=list(PATTERNS), choices=list(PATTERNS))
    parser.add_argument("--families", nargs="*", default=["terrain", "overlay", "gallery"],
                        choices=["terrain", "overlay", "gallery", "scene", "sprites"])
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
    binary = Path(args.render_binary)
    fixture = out_dir / "base-fixture.json"
    if not fixture.exists():
        fixture = make_base_fixture(binary, out_dir, args.seed, args.players)
    (out_dir / "run.json").write_text(json.dumps(vars(args), indent=2))
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
    if "sprites" in args.families:
        cases.extend(sprite_cases(args.zooms))
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

    cpu_records = capture_mode(binary, out_dir, "cpu", fixture, indexed_cases, 0, args.reveal_radius, args.legacy_reference)
    gpu_records = capture_mode(binary, out_dir, "gpu", fixture, indexed_cases, 0, args.reveal_radius)
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
            record = {"file": out_png.name, "case": cpu_record["case"], "gpu": gpu_record.get("gpu", {}), "coverage": gpu_record.get("coverage"), **accuracy_record}
            f.write(json.dumps(record, sort_keys=True) + "\n")
            print(f"{record['file']} pixel_accuracy={accuracy_record['pixel_accuracy']:.4f}%")
    accuracy_records = [accuracy_by_file[name] for name in sorted(accuracy_by_file)]
    with accuracy_path.open("w") as f:
        json.dump({"results": accuracy_records}, f, indent=2, sort_keys=True)
    records = [json.loads(line) for line in manifest.read_text().splitlines()]
    rows = []
    for record in records:
        title = html.escape(record["case"])
        if record.get("skipped"):
            rows.append(f"<article><h2>{title}</h2><p>Unsupported fixture/pose; not accepted.</p></article>")
        else:
            filename = html.escape(record["file"])
            details = html.escape(json.dumps({k: record.get(k) for k in ("gpu", "coverage")}, indent=2))
            rows.append(f'<article><h2>{title}</h2><a href="{filename}"><img loading="lazy" src="{filename}"></a><details><summary>GPU coverage</summary><pre>{details}</pre></details></article>')
    (out_dir / "index.html").write_text('<!doctype html><meta charset="utf-8"><title>Renderer acceptance gallery</title><style>body{font:16px system-ui;background:#171717;color:#eee;margin:24px}img{max-width:100%}article{margin:36px 0}pre{white-space:pre-wrap}</style><h1>Renderer acceptance gallery</h1><p>CPU composition left; whole-map GPU right. Both use modern atlases unless --legacy-reference is selected. Matching background pixels are not proof of sprite correctness. Review silhouettes, anchors, facing, fog and transparency. Unsupported poses remain explicit.</p>' + ''.join(rows))
    coverage_rows = ["# GPU coverage from captured scenes", "",
                     "Counts describe changed map cells in an explicit rebuild, not frame time or total GPU utilization.", "",
                     "| Scene | GPU raster cells | CPU-composited cells | Sprite fallback |", "|---|---:|---:|---|"]
    for record in records:
        coverage = record.get("coverage") or {}
        gpu = record.get("gpu") or {}
        if not record.get("skipped"):
            coverage_rows.append(f"| {record['case']} | {coverage.get('raster_cells', 'unknown')} | {coverage.get('cpu_cells', 'unknown')} | {gpu.get('sprite_fallback_reason') or 'none reported'} |")
    (out_dir / "coverage.md").write_text("\n".join(coverage_rows) + "\n")
    count = len(cpu_records)
    print(f"wrote {count} review images to {out_dir}")
    print(f"wrote pixel accuracy to {accuracy_path}")


if __name__ == "__main__":
    main()
