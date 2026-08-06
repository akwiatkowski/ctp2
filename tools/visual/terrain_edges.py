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
import time
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

# Render paths under test. "cpu" is the reference every other mode is compared
# against. CTP2_GPU_WORLDMAP implies quads (there is nothing to draw into the
# whole-map target without them), but it is set explicitly so the intent is
# readable here rather than inferred from engine defaults.
MODE_ENV = {
    "cpu":      {"CTP2_GPU_QUADS": "0", "CTP2_GPU_WORLDMAP": "0"},
    "gpu":      {"CTP2_GPU_QUADS": "1", "CTP2_GPU_WORLDMAP": "0"},
    "worldmap": {"CTP2_GPU_QUADS": "1", "CTP2_GPU_WORLDMAP": "1"},
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


MIN_TERRAIN_PIXELS = 500


def capture_settled(client, center, path, crop_size,
                    attempts=40, delay=0.25, before_each=None):
    """Capture once the frame has STOPPED CHANGING, not merely started drawing.

    The whole-map target is dirty-tracked and composited during Refresh, so a
    screenshot taken straight after centring can catch it mid-build: the same
    configuration measured 0 world pixels in one run and 25407 in another,
    decided purely by timing. Re-centring each attempt drives the redraw (the
    approach pan_pixel_proof.py uses).

    Waiting for "has some terrain" is not enough — a frame that just crossed a
    pixel count can still be half-composited, which is what kept the coverage
    numbers swinging between runs. Requiring two consecutive identical captures
    is the criterion that actually means finished.

    before_each runs on every attempt, not once before the loop. Visibility
    DECAYS, and this loop can spend ten seconds settling — so a reveal applied
    only beforehand has partly worn off by the time the frame is captured, and
    by different amounts in each mode's process. That was invisible while the
    whole-map path ignored fog; once it started drawing fog (#12839) it became
    the largest source of divergence in the run, and it showed up on whichever
    pair happened to be captured last.

    Every mode goes through this, so no mode is compared against a partial or a
    differently-lit frame.
    """
    previous = None
    for _ in range(attempts):
        if before_each:
            before_each()
        client.expect_ok("camera_debug_center", center["x"], center["y"])
        client.expect_ok("screenshot_presented", path)
        current = Path(path).read_bytes()
        if (previous == current
                and terrain_profile(path, crop_size)["world_pixels"] >= MIN_TERRAIN_PIXELS):
            return True
        previous = current
        time.sleep(delay)
    return False


def capture_mode(binary, out_dir, mode_name, seed, players, pairs, patterns, radius,
                 crop_size, reveal_radius, save_fixture=None, load_fixture=None,
                 lit=False):
    env = os.environ.copy()
    env["CTP2_GPU_LAYERS"] = "1"
    env["CTP2_GPU_CAMERA"] = "1"
    env.update(MODE_ENV[mode_name])
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
        # Hold fog off for the whole run rather than revealing a radius per
        # capture. Visibility DECAYS, so a reveal wears off during the seconds a
        # capture takes to settle, and by different amounts in each mode's
        # process. That was harmless while the whole-map path ignored fog; once
        # it started drawing fog (#12839) it became the biggest difference in
        # the run and landed on whichever pair was captured last. This flag does
        # not decay, and fog has its own test now (worldmap-fog).
        if lit:
            client.expect_ok("debug_render_explored_as_visible", 1)

        lookup = terrain_lookup(client)
        center = visible_center(client)
        client.expect_ok("camera_debug_center", center["x"], center["y"])

        # Warm-up capture, discarded. The FIRST measured capture would otherwise
        # be racing start-up: the renderer, the tile atlas and the whole-map
        # target are all still coming up, and capture_settled's 10s budget was
        # occasionally spent before any terrain appeared at all — the run then
        # died with "never reached 500 terrain pixels", which looks like a
        # rendering failure and is not one. Absorbing that here costs one
        # capture and makes every measured one start from a live renderer.
        if reveal_radius > 0:
            client.expect_ok("debug_reveal_patch", center["x"], center["y"],
                             reveal_radius)
        capture_settled(client, center, str(out_dir / f"{mode_name}-warmup.bmp"),
                        crop_size)

        for pair_spec in pairs:
            terrain_a, terrain_b = resolve_pair(lookup, pair_spec)
            for pattern in patterns:
                paint_patch(client, center, terrain_a, terrain_b, pattern, radius)
                # Make the compared area VISIBLE, not merely explored. The quad
                # path filters cells on m_localVision->IsVisible; the whole-map
                # path does not, because fog there is P13 step 4 and unbuilt.
                # Without this the run measures that difference instead of the
                # transitions it exists to measure.
                #
                # Re-applied per capture, not once at startup: visibility decays
                # over a sequence of captures, and revealing only at the start
                # left the reference 53% lit by the end of a 15-capture run
                # while a single-capture run measured 99%.
                reveal = None
                if reveal_radius > 0:
                    def reveal():
                        client.expect_ok("debug_reveal_patch", center["x"],
                                         center["y"], reveal_radius)
                    reveal()
                path = out_dir / f"{mode_name}-{pair_spec.replace(':', '-')}-{pattern}.bmp"
                # Both mechanisms, on every attempt. --lit steadies the
                # whole-map path, but the reference path does not honour it
                # uniformly -- several of its draw sites test IsVisible alone --
                # so its brightness still follows decaying visibility and has to
                # be topped up as the frame settles.
                if not capture_settled(client, center, path, crop_size,
                                       before_each=reveal):
                    raise RuntimeError(
                        f"{mode_name} {pair_spec} {pattern}: frame never reached "
                        f"{MIN_TERRAIN_PIXELS} terrain pixels — capturing it would "
                        f"compare a half-built frame")
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
        if shot["mode"] != "cpu":
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


def crop_box(w, h, crop_size):
    crop_w = min(crop_size, w) if crop_size > 0 else w
    crop_h = min(crop_size, h) if crop_size > 0 else h
    return (w - crop_w) // 2, (h - crop_h) // 2, crop_w, crop_h


def spatial_diff(path_a, path_b, crop_size, channel_tolerance):
    """Per-pixel agreement between two crops of identical dimensions.

    The colour-ratio profile above is a HISTOGRAM: it counts how much green and
    yellow a frame contains, but not WHERE. A transition bug that composites the
    right terrains in the wrong arrangement — tiles showing fragments of their
    neighbours — moves pixels around without changing the mix, and slips through
    a histogram unchanged. Comparing position by position is what actually
    catches it.

    A tolerance is applied per channel because the two paths blend and round
    independently; only differences beyond it count as moved pixels.
    """
    wa, ha, pa = read_bmp_rgb(path_a)
    wb, hb, pb = read_bmp_rgb(path_b)
    if (wa, ha) != (wb, hb):
        raise ValueError(f"frame size mismatch: {wa}x{ha} vs {wb}x{hb}")

    left, top, cw, ch = crop_box(wa, ha, crop_size)
    compared = differing = 0
    total_error = 0
    for y in range(top, top + ch):
        for x in range(left, left + cw):
            ra, ga, ba = pa[y][x]
            rb, gb, bb = pb[y][x]
            # Skip pixels that are unexplored black in BOTH frames: they carry
            # no terrain and would dilute the ratio toward a free pass.
            if max(ra, ga, ba) <= 8 and max(rb, gb, bb) <= 8:
                continue
            compared += 1
            delta = max(abs(ra - rb), abs(ga - gb), abs(ba - bb))
            total_error += delta
            if delta > channel_tolerance:
                differing += 1
    return {
        "compared": compared,
        "differing": differing,
        "diff_ratio": differing / compared if compared else 1.0,
        "mean_error": total_error / compared if compared else 255.0,
    }


def check_modes_vs_reference(shots, reference, crop_size, min_world_coverage_ratio,
                             max_color_ratio_delta, max_diff_ratio, channel_tolerance):
    """Compare every captured mode against `reference`.

    Which reference to use is not cosmetic. Against "cpu" the spatial metric is
    useless today: the shipping quad path scores diff_ratio 0.956 vs CPU and the
    whole-map path 0.952 — indistinguishable, because both are swamped by the
    known CPU/GPU framing offset. Against "gpu" the two paths share the quad
    machinery and the framing, so what remains is the P13 change itself.
    """
    by_key = {(s["mode"], s["pair"], s["pattern"]): s for s in shots}
    failures = []
    for shot in shots:
        mode = shot["mode"]
        if mode == reference:
            continue
        label = f"{mode} vs {reference} {shot['pair']} {shot['pattern']}"
        ref = by_key.get((reference, shot["pair"], shot["pattern"]))
        if not ref:
            failures.append(f"missing {reference} reference for {label}")
            continue
        if mode != "cpu" and (not shot["gpu"].get("enabled")
                              or not shot["gpu"].get("complete")):
            failures.append(f"GPU frame incomplete for {label}: {shot['gpu']}")
            continue
        # Without this the whole-map run could silently fall back to the
        # ADR-002 path and "pass" by comparing that path against the CPU
        # reference — a green light for a renderer nobody tested.
        # Both fields are required. The flag says the path was REQUESTED; the
        # texture says the present actually sampled the whole-map target rather
        # than falling through to the window mirror. Checking only the flag is
        # how this test first came up green while measuring nothing.
        if mode == "worldmap" and not (shot["gpu"].get("worldmap")
                                       and shot["gpu"].get("worldmap_texture")):
            failures.append(
                f"{label}: the whole-map path was NOT under test — "
                f"worldmap={shot['gpu'].get('worldmap')} "
                f"worldmap_texture={shot['gpu'].get('worldmap_texture')}; "
                f"the present fell back to the ADR-002 window mirror")
            continue

        profile = terrain_profile(shot["path"], crop_size)
        ref_profile = terrain_profile(ref["path"], crop_size)
        coverage_ratio = (
            profile["world_pixels"] / ref_profile["world_pixels"]
            if ref_profile["world_pixels"] else 0.0
        )
        color_delta = max(
            abs(profile["green_ratio"] - ref_profile["green_ratio"]),
            abs(profile["yellow_ratio"] - ref_profile["yellow_ratio"]),
            abs(profile["dark_ratio"] - ref_profile["dark_ratio"]),
        )
        spatial = spatial_diff(shot["path"], ref["path"], crop_size, channel_tolerance)
        print(
            f"{label}: coverage={coverage_ratio:.3f} "
            f"color_delta={color_delta:.3f} "
            f"diff_ratio={spatial['diff_ratio']:.3f} "
            f"mean_error={spatial['mean_error']:.1f} "
            f"world={profile['world_pixels']} ref_world={ref_profile['world_pixels']}"
        )
        if coverage_ratio < min_world_coverage_ratio or color_delta > max_color_ratio_delta:
            failures.append(
                f"{label} exceeds threshold: "
                f"coverage={coverage_ratio:.3f}/{min_world_coverage_ratio}, "
                f"color_delta={color_delta:.3f}/{max_color_ratio_delta}"
            )
        if max_diff_ratio is not None and spatial["diff_ratio"] > max_diff_ratio:
            failures.append(
                f"{label} pixels disagree spatially: "
                f"diff_ratio={spatial['diff_ratio']:.3f}/{max_diff_ratio} "
                f"({spatial['differing']} of {spatial['compared']} pixels differ "
                f"by more than {channel_tolerance}/255)"
            )
    if failures:
        raise SystemExit("terrain parity check failed:\n" + "\n".join(failures))


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
    parser.add_argument("--modes", nargs="+", default=["gpu", "cpu"],
                        choices=sorted(MODE_ENV),
                        help="render paths to capture; 'cpu' is the reference "
                             "every other mode is checked against")
    parser.add_argument("--reference", default="cpu", choices=sorted(MODE_ENV),
                        help="mode every other mode is compared against. Use "
                             "'gpu' for spatial checks: against 'cpu' the "
                             "known framing offset swamps the metric")
    parser.add_argument("--check", action="store_true",
                        help="fail if a mode's crops differ from the reference "
                             "beyond thresholds")
    parser.add_argument("--min-world-coverage-ratio", type=float, default=0.45)
    parser.add_argument("--max-color-ratio-delta", type=float, default=0.15)
    parser.add_argument("--max-diff-ratio", type=float, default=None,
                        help="max fraction of terrain pixels allowed to differ "
                             "POSITIONALLY from CPU. Off by default: the older "
                             "gpu-vs-cpu comparison is a histogram check and "
                             "was never calibrated against this.")
    parser.add_argument("--lit", action="store_true",
                        help="render explored terrain at full brightness for "
                             "the whole run, taking fog out of the comparison "
                             "deterministically. Prefer this to --reveal-radius: "
                             "revealing decays during a capture and the two "
                             "processes drift apart")
    parser.add_argument("--reveal-radius", type=int, default=0,
                        help="make this radius around the compared area VISIBLE "
                             "(not just explored) before capturing. Needed for a "
                             "fair transition comparison: the quad path hides "
                             "non-visible cells and the whole-map path does not, "
                             "so without it the run measures fog, not transitions")
    parser.add_argument("--channel-tolerance", type=int, default=24,
                        help="per-channel delta below which a pixel counts as "
                             "matching, absorbing independent blend/rounding drift")
    parser.add_argument("--crop-size", type=int, default=420,
                        help="center crop size before contact-sheet scaling")
    parser.add_argument("--scale", type=int, default=2,
                        help="nearest-neighbor upscale for inspecting tile edges")
    args = parser.parse_args()

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    binary = Path(args.binary)

    modes = list(dict.fromkeys(args.modes))   # de-dupe, keep order
    if args.check and (args.reference not in modes or len(modes) < 2):
        raise SystemExit(
            f"--check needs the reference mode {args.reference!r} plus at "
            f"least one other mode; got {modes}")

    all_shots = []
    fixture = out_dir / "terrain-edge-fixture.json"
    for index, mode in enumerate(modes):
        # Every mode must see the SAME map, or the comparison is meaningless:
        # the first run saves the generated world and the rest load it.
        save_fixture = str(fixture) if args.check and index == 0 else None
        load_fixture = (str(fixture) if args.check and index > 0
                        and fixture.exists() else None)
        all_shots.extend(capture_mode(binary, out_dir, mode, args.seed,
                                      args.players, args.pairs,
                                      args.patterns, args.radius,
                                      args.crop_size, args.reveal_radius,
                                      save_fixture=save_fixture,
                                      load_fixture=load_fixture,
                                      lit=args.lit))
    write_contact_sheet(out_dir, all_shots, args.crop_size, args.scale)
    if args.check:
        check_modes_vs_reference(all_shots, args.reference, args.crop_size,
                                 args.min_world_coverage_ratio,
                                 args.max_color_ratio_delta,
                                 args.max_diff_ratio,
                                 args.channel_tolerance)


if __name__ == "__main__":
    main()
