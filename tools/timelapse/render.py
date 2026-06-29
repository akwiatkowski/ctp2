#!/usr/bin/env python3
"""Render an AI-empire timelapse from a record.py JSONL into PNG frames + mp4.

The RENDER half of the record-then-render pipeline. Reads the snapshot log and
draws, per turn:
  - terrain tiles (water/mountain/land, land shaded by food richness)
  - territory: each city tints a soft diamond in its owner's colour -> empires
    show up as growing coloured blobs (an approximation of borders from city
    positions, no per-tile owner needed)
  - cities as dots (owner colour, radius grows with population)
  - HUD: turn / year + a score leaderboard by civ name, protagonist spotlit
  - a Chronicle caption strip: the salient action-log events for that turn

Then ffmpeg stitches the frames into an mp4.

Usage:
    render.py [in.jsonl] [out_dir]

Env:
    TIMELAPSE_PROTAGONIST   player id to spotlight (default: top scorer, final frame)
    TILE                    pixels per map tile (default 9)
    TERRITORY_RADIUS        city influence radius in tiles (default 3)
    FPS                     output framerate (default 12)
"""

import json
import os
import re
import sys
import subprocess

from PIL import Image, ImageDraw, ImageFont

IN = sys.argv[1] if len(sys.argv) > 1 else "/tmp/ctp2-timelapse.jsonl"
OUT_DIR = sys.argv[2] if len(sys.argv) > 2 else "/tmp/ctp2-timelapse-frames"
TILE = int(os.environ.get("TILE", "9"))
TERRITORY_RADIUS = int(os.environ.get("TERRITORY_RADIUS", "3"))
FPS = int(os.environ.get("FPS", "12"))
PROTAGONIST_ENV = os.environ.get("TIMELAPSE_PROTAGONIST")

HUD_W = 250            # right-hand HUD panel width (px)
CAP_H = 64             # bottom Chronicle caption strip height (px)
BG = (16, 18, 24)

PLAYER_COLORS = [
    (224, 64, 64), (64, 128, 240), (80, 200, 96), (236, 200, 64), (200, 96, 220),
    (96, 216, 216), (240, 144, 56), (200, 200, 208), (160, 110, 70), (240, 120, 170),
]

# Action-log event name -> (priority, caption verb). Lower priority shows first.
# ONLY these events are captioned; the firehose of EntrenchOrder / MoveOrder /
# SettleOrder / BoardTransportOrder / MakePop / CreateUnit is military micro-
# management noise and is dropped entirely so the Chronicle reads as a story.
EVENT_CAPTIONS = {
    "CreateCity":          (0, "founds a city"),
    "GrantAdvance":        (1, "discovers a new technology"),
    "NewProposal":         (1, "proposes a deal"),
    "Threaten":            (1, "issues a threat"),
    "KillUnit":            (2, "loses a unit in battle"),
    "CreateBuilding":      (2, "completes a building"),
    "ProposalResponse":    (3, "answers a proposal"),
    "Reject":              (3, "rejects a proposal"),
    "ImprovementComplete": (4, "finishes construction"),
}


def player_color(pid):
    return PLAYER_COLORS[pid % len(PLAYER_COLORS)]


def load_font(size):
    for path in ("/System/Library/Fonts/Supplemental/Arial.ttf",
                 "/System/Library/Fonts/Helvetica.ttc",
                 "/Library/Fonts/Arial.ttf"):
        try:
            return ImageFont.truetype(path, size)
        except Exception:
            pass
    return ImageFont.load_default()


def terrain_palette(meta):
    """{terrain_id: (r,g,b)} from query_terrains; land shaded green->tan by food."""
    pal = {}
    terr = (meta or {}).get("terrains") if meta else None
    rows = (terr or {}).get("terrains") if isinstance(terr, dict) else None
    if not rows:
        return pal
    foods = [r.get("food", 0) for r in rows if r.get("land")]
    fmax = max(foods) if foods else 1
    for r in rows:
        tid = r.get("id")
        if tid is None:
            continue
        if r.get("water"):
            pal[tid] = (40, 70, 130)
        elif r.get("mountain"):
            pal[tid] = (110, 110, 120)
        elif r.get("land"):
            f = (r.get("food", 0) / fmax) if fmax else 0.0
            lush, poor = (70, 150, 70), (170, 150, 100)
            pal[tid] = tuple(int(poor[i] + (lush[i] - poor[i]) * f) for i in range(3))
        else:
            pal[tid] = (60, 64, 72)
    return pal


def frame_captions(events):
    """This frame's events -> [(player_id, verb)], curated beats only (max 3)."""
    picked = []
    for e in events or []:
        cap = EVENT_CAPTIONS.get(e.get("event"))
        if cap:
            picked.append((cap[0], e.get("player"), cap[1]))
    picked.sort(key=lambda x: x[0])                # by priority
    return [(pid, verb) for _, pid, verb in picked[:3]]


def pick_protagonist(frames):
    """Default: the widest empire (most cities, score as tiebreak) — a sprawling
    map makes the best timelapse. Override with TIMELAPSE_PROTAGONIST."""
    if PROTAGONIST_ENV is not None:
        return int(PROTAGONIST_ENV)
    last = frames[-1]
    alive = [p for p in (last.get("players") or []) if not p.get("dead")]
    top = max(alive, key=lambda p: (p.get("cities", 0), p.get("score", 0)), default=None)
    return top["id"] if top else 0


def civ_label(p):
    return p.get("civ") or p.get("name") or f"p{p['id']}"


def render_frame(frame, pal, protagonist, show_year, civ_by_pid, fonts):
    font, font_sm, font_cap = fonts
    W, H = frame["width"], frame["height"]
    terrain = frame["terrain"] or []
    img_w = W * TILE + HUD_W
    img_h = H * TILE + CAP_H
    img = Image.new("RGB", (img_w, img_h), BG)
    px = img.load()

    # 1) terrain
    for y in range(H):
        row = y * W
        for x in range(W):
            tid = terrain[row + x] if row + x < len(terrain) else -1
            col = pal.get(tid, (50, 54, 62))
            x0, y0 = x * TILE, y * TILE
            for dy in range(TILE):
                for dx in range(TILE):
                    px[x0 + dx, y0 + dy] = col

    draw = ImageDraw.Draw(img, "RGBA")

    # 2) territory tint (soft diamond per city; protagonist strong, rivals faint)
    for c in frame.get("cities") or []:
        owner = c["owner"]
        col = player_color(owner)
        peak = 0.55 if owner == protagonist else 0.26
        cx, cy, R = c["x"], c["y"], TERRITORY_RADIUS
        for dy in range(-R, R + 1):
            for dx in range(-R, R + 1):
                man = abs(dx) + abs(dy)
                if man > R:
                    continue
                tx, ty = cx + dx, cy + dy
                if 0 <= tx < W and 0 <= ty < H:
                    a = peak * (1 - man / (R + 1))
                    draw.rectangle([tx * TILE, ty * TILE, tx * TILE + TILE - 1,
                                    ty * TILE + TILE - 1], fill=col + (int(a * 255),))

    # 3) city dots (radius ~ pop); protagonist ringed white
    for c in frame.get("cities") or []:
        owner = c["owner"]
        col = player_color(owner)
        cx = c["x"] * TILE + TILE // 2
        cy = c["y"] * TILE + TILE // 2
        r = max(2, min(TILE, 2 + int((c.get("pop", 1) or 1) ** 0.5)))
        draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=col,
                     outline=(255, 255, 255) if owner == protagonist else (0, 0, 0))

    # 4) HUD
    panel_x = W * TILE
    draw.rectangle([panel_x, 0, img_w, img_h], fill=(10, 12, 16))
    year = frame.get("year")
    clock = f"turn {frame['turn']}"
    if show_year and year is not None:
        clock += f"   {abs(year)} {'BC' if year < 0 else 'AD'}"
    draw.text((panel_x + 12, 12), "CALL TO POWER 2", font=font, fill=(235, 235, 245))
    draw.text((panel_x + 12, 36), clock, font=font_sm, fill=(170, 180, 200))

    players = sorted((frame.get("players") or []), key=lambda p: p.get("score", 0), reverse=True)
    yoff = 80
    draw.text((panel_x + 12, yoff - 22), "civ", font=font_sm, fill=(120, 130, 150))
    draw.text((panel_x + 150, yoff - 22), "score  cty", font=font_sm, fill=(120, 130, 150))
    for p in players:
        if p.get("dead") or (p.get("cities", 0) == 0 and p.get("score", 0) == 0):
            continue
        col = player_color(p["id"])
        prot = p["id"] == protagonist
        draw.rectangle([panel_x + 12, yoff + 3, panel_x + 24, yoff + 15], fill=col)
        draw.text((panel_x + 32, yoff), civ_label(p)[:14], font=font_sm,
                  fill=(255, 255, 255) if prot else (160, 168, 184))
        draw.text((panel_x + 150, yoff), f"{p.get('score',0):>5} {p.get('cities',0):>3}",
                  font=font_sm, fill=(255, 255, 255) if prot else (160, 168, 184))
        if prot:
            draw.text((panel_x + 232, yoff), "*", font=font_sm, fill=(255, 230, 120))
        yoff += 20

    # 5) Chronicle caption strip (bottom): this round's salient events
    cap_y = H * TILE
    draw.rectangle([0, cap_y, img_w, img_h], fill=(8, 9, 13))
    evs = frame_captions(frame.get("events"))
    if evs:
        ty = cap_y + 8
        for pid, verb in evs:
            if isinstance(pid, int) and 0 <= pid < 32:
                draw.rectangle([10, ty + 2, 22, ty + 14], fill=player_color(pid))
                who = civ_by_pid.get(pid, f"p{pid}")
            else:
                who = ""
            draw.text((30, ty), f"{who} {verb}".strip(), font=font_cap, fill=(210, 214, 228))
            ty += 17
    else:
        draw.text((30, cap_y + 8), "· · ·", font=font_cap, fill=(80, 86, 100))
    return img


def main():
    frames, meta = [], None
    with open(IN) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            rec = json.loads(line)
            t = rec.get("type")
            if t == "meta":
                meta = rec
            elif t == "frame":
                frames.append(rec)
    if not frames:
        print(f"[RENDER] no frames in {IN}")
        return 1

    pal = terrain_palette(meta)
    protagonist = pick_protagonist(frames)
    # Only show a BC/AD year if it actually varies — the autoplay path leaves
    # the engine year frozen, in which case "turn N" alone is the honest clock.
    show_year = len({f.get("year") for f in frames if f.get("year") is not None}) > 1
    n_events = sum(len(f.get("events") or []) for f in frames)
    civ_by_pid = {p["id"]: civ_label(p) for p in (frames[-1].get("players") or [])}
    fonts = (load_font(17), load_font(13), load_font(13))
    os.makedirs(OUT_DIR, exist_ok=True)
    prot_name = civ_by_pid.get(protagonist, f"p{protagonist}")
    print(f"[RENDER] {len(frames)} frames, protagonist={prot_name}, "
          f"{n_events} events, year={'live' if show_year else 'frozen->turns only'}, "
          f"tile={TILE}px -> {OUT_DIR}")

    for i, fr in enumerate(frames):
        img = render_frame(fr, pal, protagonist, show_year, civ_by_pid, fonts)
        img.save(os.path.join(OUT_DIR, f"frame_{i:04d}.png"))
        if i % 25 == 0:
            print(f"[RENDER] {i}/{len(frames)}")

    mp4 = os.path.join(OUT_DIR, "timelapse.mp4")
    cmd = ["ffmpeg", "-y", "-framerate", str(FPS),
           "-i", os.path.join(OUT_DIR, "frame_%04d.png"),
           "-c:v", "libx264", "-pix_fmt", "yuv420p",
           "-vf", "scale=trunc(iw/2)*2:trunc(ih/2)*2", mp4]
    print("[RENDER] ffmpeg ->", mp4)
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr[-1500:])
        return 1
    print(f"[RENDER] done: {mp4}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
