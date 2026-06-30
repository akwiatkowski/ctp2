#!/usr/bin/env python3
"""Record per-turn UNFOGGED world snapshots from an AI-vs-AI autoplay game.

Drives the game over the smoke socket (new_game / start_game /
enable_autoplay / advance_round), but each turn it asks the engine for the
omniscient `query_world` snapshot (+ `query_turn`) and appends it to a JSONL file.

This is the RECORD half of a record-then-render pipeline: the (slow, fragile)
game runs once and produces a compact log; render.py then turns that log into a
timelapse offline, so the visual can be iterated on without replaying.

Output JSONL:
  line 1 : {"type":"meta",  "terrains":{...}, "target_turns":N}
  line k : {"type":"frame", "turn":i, "round":r, "year":y,
            "width":W, "height":H, "terrain":[...row-major terrain ids...],
            "cities":[{"owner","x","y","pop"}...],
            "players":[{"id","cities","dead","score"}...]}

Env:
  CTP2_BINARY            game binary (default: build/ctp2)
  CTP2_CWD               game data dir (default: walk up to appstr.txt)
  AUTOPLAY_TURNS         turns to run (default 150)
  AUTOPLAY_TURN_PACE     seconds to wait after each advance_round (default 2.0)
  AUTOPLAY_TURN_TIMEOUT  per-advance_round socket timeout (default 60)
  TIMELAPSE_OUT          output JSONL (default /tmp/ctp2-timelapse.jsonl)
  SNAPSHOT_INTERVAL      record a frame every N turns (default 1)
"""

import json
import os
import socket
import subprocess
import sys
import time

GAME_BINARY = os.path.abspath(
    os.environ.get("CTP2_BINARY")
    or os.path.join(os.path.dirname(__file__), "..", "..", "build", "ctp2")
)
SOCKET_PATH = "/tmp/ctp2-smoke.sock"
TIMEOUT_INIT = 30
TURNS = int(os.environ.get("AUTOPLAY_TURNS", "150"))
TURN_PACE = float(os.environ.get("AUTOPLAY_TURN_PACE", "2.0"))
TURN_TIMEOUT = int(os.environ.get("AUTOPLAY_TURN_TIMEOUT", "60"))
OUT = os.environ.get("TIMELAPSE_OUT", "/tmp/ctp2-timelapse.jsonl")
INTERVAL = max(1, int(os.environ.get("SNAPSHOT_INTERVAL", "1")))
GAME_LOG = "/tmp/ctp2-timelapse-game.log"
# Real-art mode: each turn ask the engine for an unfogged isometric BMP of the
# whole map (render_map). The BMP is the frame's map layer; render.py composites
# the HUD + Chronicle over it. ZOOM 0..5 trades file size for tile detail.
REALART = os.environ.get("TIMELAPSE_REALART", "1") != "0"
RENDER_ZOOM = int(os.environ.get("TIMELAPSE_ZOOM", "1"))
RAW_DIR = os.path.join(os.path.dirname(os.path.abspath(OUT)), "raw")
# TIMELAPSE_PLAYER: render the FOGGED view from this player's perspective
# (cities/units/infra + fog-of-war, cropped to what they've seen). Empty/-1 =
# the unfogged god's-eye whole map. Default -1 shows the whole AI-vs-AI run.
_pl = os.environ.get("TIMELAPSE_PLAYER", "-1")
PLAYER = None if _pl in ("", "-1", "all") else int(_pl)


def send_cmd(cmd: str, timeout: float = 20) -> dict:
    """Send one {"cmd": ...} line, read one line of JSON back.

    query_world payloads are large (a full terrain grid), so read with a
    generous buffer until the terminating newline arrives.
    """
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.settimeout(timeout)
    try:
        sock.connect(SOCKET_PATH)
        sock.sendall((json.dumps({"cmd": cmd}) + "\n").encode())
        buf = b""
        while b"\n" not in buf:
            chunk = sock.recv(65536)
            if not chunk:
                break
            buf += chunk
        return json.loads(buf.decode().strip().split("\n")[0])
    finally:
        sock.close()


def bmp_to_png(bmp: str):
    """Convert the engine's BMP to a PNG and delete the BMP. render_map emits
    ~11MB BMPs; PNG is ~10x smaller, so a long run stays a few hundred MB
    instead of multiple GB. Returns the PNG path (or the BMP if conversion
    isn't possible)."""
    try:
        from PIL import Image
        png = bmp[:-4] + ".png"
        Image.open(bmp).save(png)
        os.remove(bmp)
        return png
    except Exception as e:
        print(f"[REC] png convert failed ({e}); keeping BMP")
        return bmp


def wait_for_socket(path: str, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if os.path.exists(path):
            return
        time.sleep(0.1)
    raise TimeoutError(f"Socket {path} did not appear within {timeout}s")


def main() -> int:
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)

    game_cwd = os.environ.get("CTP2_CWD")
    if not game_cwd:
        candidate = os.path.dirname(os.path.dirname(GAME_BINARY))
        while candidate and candidate != "/":
            if os.path.exists(os.path.join(candidate, "appstr.txt")):
                game_cwd = candidate
                break
            candidate = os.path.dirname(candidate)

    print(f"[REC] Launching {GAME_BINARY} cwd={game_cwd} turns={TURNS} -> {OUT}")
    log_file = open(GAME_LOG, "w")
    proc = subprocess.Popen(
        [GAME_BINARY, "smoke-test"],
        stdout=log_file,
        stderr=subprocess.STDOUT,
        cwd=game_cwd,
    )

    try:
        wait_for_socket(SOCKET_PATH, TIMEOUT_INIT)
    except TimeoutError as e:
        print(f"[REC] FAIL init: {e}")
        proc.terminate(); proc.wait(); log_file.close()
        return 1

    print("[REC] Game up")
    out = open(OUT, "w")
    frames = 0
    if REALART:
        os.makedirs(RAW_DIR, exist_ok=True)

    def cmd(c, timeout=20):
        try:
            return send_cmd(c, timeout=timeout)
        except Exception as e:
            print(f"[REC] cmd {c!r} failed: {e}")
            return None

    cmd("new_game")
    cmd("start_game")
    time.sleep(5)
    print(f"[REC] autoplay: {cmd('enable_autoplay')}")

    # Meta line: terrain drives colours; names let offline render resolve
    # action-log ids into Chronicle captions without talking to the engine.
    terrains = cmd("query_terrains")
    names = cmd("query_names")
    out.write(json.dumps({"type": "meta",
                          "terrains": terrains.get("result", terrains) if terrains else None,
                          "names": names.get("result", names) if names else None,
                          "target_turns": TURNS}) + "\n")
    out.flush()

    # The action log is global. Clear it between frames so Chronicle captions
    # describe the round that produced the current snapshot.
    cmd("log_clear")

    for i in range(TURNS):
        if proc.poll() is not None:
            print(f"[REC] game exited early at turn {i} (code={proc.returncode})")
            break
        t0 = time.time()
        cmd("advance_round", timeout=TURN_TIMEOUT)
        time.sleep(TURN_PACE)

        turn_num = i + 1
        if turn_num % INTERVAL != 0:
            continue

        world = cmd("query_world", timeout=30)
        clock = cmd("query_turn", timeout=10)
        # Events that fired since the last clear == this snapshot's events.
        logr = cmd("log_get", timeout=20)
        cmd("log_clear")
        if not world or world.get("status") != "ok":
            print(f"[REC] turn {turn_num}: query_world -> {world}")
            continue
        w = world["result"] if "result" in world else world
        c = (clock.get("result", clock) if clock else {}) or {}
        events = ((logr.get("result", logr) if logr else {}) or {}).get("action_log") or []

        img_path = None
        crop = None
        cities_labels = None
        if REALART:
            bmp = os.path.join(RAW_DIR, f"turn-{turn_num:04d}.bmp")
            if PLAYER is not None:
                r = cmd(f"render_map_player {PLAYER} {bmp} {RENDER_ZOOM}", timeout=60)
            else:
                r = cmd(f"render_map {bmp} {RENDER_ZOOM}", timeout=60)
            if r and r.get("status") == "ok":
                img_path = bmp_to_png(bmp)
                # detail: "crop=x,y,w,h cities=px~py~owner~pop~name;..."
                d = r.get("detail") or ""
                head, _, citystr = d.partition(" cities=")
                if head.startswith("crop="):
                    try:
                        crop = [int(v) for v in head[5:].split(",")]
                    except Exception:
                        crop = None
                if citystr:
                    cities_labels = []
                    for tok in citystr.split(";"):
                        f = tok.split("~", 4)   # px, py, owner, pop, name
                        if len(f) == 5:
                            try:
                                cities_labels.append({"px": int(f[0]), "py": int(f[1]),
                                                      "owner": int(f[2]), "pop": int(f[3]),
                                                      "name": f[4]})
                            except Exception:
                                pass
            else:
                print(f"[REC] turn {turn_num}: render -> {r}")

        out.write(json.dumps({
            "type": "frame",
            "turn": turn_num,
            "round": c.get("round"),
            "year": c.get("year"),
            "width": w.get("width"),
            "height": w.get("height"),
            # Real-art frames carry the engine render (PNG); terrain/cities live
            # in it, so they are omitted from the log to keep it small.
            "img": img_path,
            "crop": crop,   # explored pixel rect [x,y,w,h] in fogged player mode
            "city_labels": cities_labels,  # [{px,py,owner,pop,name}] full-surface px
            "terrain": None if REALART else w.get("terrain"),
            "cities": None if REALART else w.get("cities"),
            "players": w.get("players"),
            "events": events,
        }) + "\n")
        out.flush()
        frames += 1
        if turn_num % 10 == 0:
            alive = [p for p in (w.get("players") or []) if not p.get("dead")]
            top = max(alive, key=lambda p: p.get("score", 0), default=None)
            lead = f"p{top['id']} score={top['score']} cities={top['cities']}" if top else "-"
            print(f"[REC] turn {turn_num}/{TURNS} wall={time.time()-t0:.1f}s frames={frames} leader={lead}")

    print(f"[REC] done: {frames} frames -> {OUT}")
    cmd("quit")
    try:
        proc.wait(timeout=15)
    except subprocess.TimeoutExpired:
        proc.terminate(); proc.wait()
    out.close(); log_file.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
