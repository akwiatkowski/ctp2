#!/usr/bin/env python3
"""
Long-form smoke test: flip all players to AI and run N turns.

Coverage tool, not a behavior test. The point is to exercise the AI/turn/
combat/diplomacy paths under ASan/UBSan. We assert nothing about outcomes —
failure means a sanitizer fired, the process crashed, or the socket timed out.

Optional coverage interleaves (set env to a positive integer to enable):
    AUTOPLAY_SAVELOAD_INTERVAL    — every N turns, save then load the game.
                                    Exercises serialization paths that pure
                                    end_turn loops never touch.
    AUTOPLAY_SCREENSHOT_INTERVAL  — every N turns, capture the primary surface
                                    to BMP. Exercises the rendering pipeline
                                    (tiledraw, sprite blitting, layers) which
                                    is otherwise idle during AI turns.

Usage:
    AUTOPLAY_TURNS=50 python3 autoplay_test.py
    AUTOPLAY_TURNS=120 AUTOPLAY_SAVELOAD_INTERVAL=40 python3 autoplay_test.py

Env vars:
    CTP2_BINARY  — path to ctp2 executable (default: build/ctp2)
    CTP2_CWD     — game data directory (default: walk up from binary)
    AUTOPLAY_TURNS — turns to run (default: 50)
    AUTOPLAY_TURN_TIMEOUT — seconds per end_turn (default: 60)
"""

import json
import os
import socket
import subprocess
import sys
import time

GAME_BINARY = (
    os.environ.get("CTP2_BINARY")
    or (sys.argv[1] if len(sys.argv) > 1 else None)
    or os.path.join(os.path.dirname(__file__), "..", "..", "build", "ctp2")
)
GAME_BINARY = os.path.abspath(GAME_BINARY)

SOCKET_PATH = os.environ.get("CTP2_SMOKE_SOCKET", "/tmp/ctp2-smoke.sock")
TIMEOUT_INIT = 30
TURNS = int(os.environ.get("AUTOPLAY_TURNS", "100"))
TURN_TIMEOUT = int(os.environ.get("AUTOPLAY_TURN_TIMEOUT", "60"))
# advance_round runs a full simulation round before returning. Keep a small
# pacing hook so callers can still slow the UI smoke build if needed.
TURN_PACE = float(os.environ.get("AUTOPLAY_TURN_PACE", "2.0"))
GAME_LOG = "/tmp/ctp2-autoplay-game.log"
SAVELOAD_INTERVAL = int(os.environ.get("AUTOPLAY_SAVELOAD_INTERVAL", "0"))
SCREENSHOT_INTERVAL = int(os.environ.get("AUTOPLAY_SCREENSHOT_INTERVAL", "0"))
SAVE_DIR = "/tmp/ctp2-autoplay-saves"
SHOT_DIR = "/tmp/ctp2-autoplay-shots"


def send_cmd(cmd: str, timeout: float = 20) -> dict:
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.settimeout(timeout)
    try:
        sock.connect(SOCKET_PATH)
        sock.sendall((json.dumps({"cmd": cmd}) + "\n").encode())
        buf = b""
        while b"\n" not in buf:
            chunk = sock.recv(4096)
            if not chunk:
                break
            buf += chunk
        return json.loads(buf.decode().strip().split("\n")[0])
    finally:
        sock.close()


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

    print(f"[AUTO] Launching {GAME_BINARY} from cwd={game_cwd}, target turns={TURNS}")
    log_file = open(GAME_LOG, "w")
    env = dict(os.environ)
    env.setdefault(
        "CTP2_PROFILE",
        os.path.join(os.path.dirname(os.path.abspath(__file__)),
                     "testprofile.txt"),
    )
    proc = subprocess.Popen(
        [GAME_BINARY, "smoke-test"],
        stdout=log_file,
        stderr=subprocess.STDOUT,
        cwd=game_cwd,
        env=env,
    )

    try:
        wait_for_socket(SOCKET_PATH, TIMEOUT_INIT)
    except TimeoutError as e:
        print(f"[AUTO] FAIL init: {e}")
        proc.terminate()
        proc.wait()
        log_file.close()
        return 1

    print("[AUTO] Game initialized")
    t0 = time.time()
    failures = 0

    def run(cmd: str, timeout: float = 20, expect_ok: bool = True) -> dict | None:
        nonlocal failures
        try:
            resp = send_cmd(cmd, timeout=timeout)
        except Exception as e:
            print(f"[AUTO] FAIL {cmd}: {e}")
            failures += 1
            return None
        if expect_ok and resp.get("status") != "ok":
            print(f"[AUTO] FAIL {cmd} -> {resp}")
            failures += 1
        return resp

    run("new_game")
    run("start_game")
    time.sleep(5)
    resp = run("enable_autoplay")
    if resp:
        print(f"[AUTO] {resp}")

    if SAVELOAD_INTERVAL > 0:
        os.makedirs(SAVE_DIR, exist_ok=True)
    if SCREENSHOT_INTERVAL > 0:
        os.makedirs(SHOT_DIR, exist_ok=True)

    for i in range(TURNS):
        if proc.poll() is not None:
            print(f"[AUTO] Game process exited unexpectedly at turn {i} (code={proc.returncode})")
            failures += 1
            break
        turn_t0 = time.time()
        run("advance_round", timeout=TURN_TIMEOUT)
        time.sleep(TURN_PACE)
        dt = time.time() - turn_t0

        # Coverage interleaves. Run *after* end_turn so the game state is
        # at a turn boundary — saves/screenshots taken mid-turn would hit
        # partially-updated AI state and produce noisy artifacts.
        turn_num = i + 1
        if SAVELOAD_INTERVAL > 0 and turn_num % SAVELOAD_INTERVAL == 0:
            path = os.path.join(SAVE_DIR, f"turn-{turn_num:04d}.c2g")
            print(f"[AUTO] save+load checkpoint at turn {turn_num} -> {path}")
            run(f"save_game {path}", timeout=60)
            run(f"load_game {path}", timeout=60)
        if SCREENSHOT_INTERVAL > 0 and turn_num % SCREENSHOT_INTERVAL == 0:
            path = os.path.join(SHOT_DIR, f"turn-{turn_num:04d}.bmp")
            run(f"screenshot {path}", timeout=20, expect_ok=False)

        if i % 10 == 0 or dt > 5:
            print(f"[AUTO] turn {turn_num}/{TURNS} wall={dt:.1f}s")

    elapsed = time.time() - t0
    # Verify we actually advanced rounds by parsing the game log.
    rounds_seen = 0
    try:
        with open(GAME_LOG) as f:
            rounds_seen = len(set(line for line in f if "BeginTurnEvent" in line and "round=" in line))
    except Exception:
        pass
    print(f"[AUTO] {TURNS} end_turn cmds in {elapsed:.1f}s, distinct BeginTurnEvents={rounds_seen}, failures={failures}")

    run("quit", expect_ok=False)
    try:
        proc.wait(timeout=15)
    except subprocess.TimeoutExpired:
        proc.terminate()
        proc.wait()

    log_file.close()
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
