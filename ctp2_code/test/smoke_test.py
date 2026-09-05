#!/usr/bin/env python3
"""
E2E smoke test for CTP2.

Launches the game in smoke-test mode, drives it through a Unix domain socket
to start a new game, settle a city, and end a turn.
"""

import json
import os
import socket
import subprocess
import sys
import time

# Game binary path: override with CTP2_BINARY env var or first CLI arg
GAME_BINARY = (
    os.environ.get("CTP2_BINARY")
    or (sys.argv[1] if len(sys.argv) > 1 else None)
    or os.path.join(os.path.dirname(__file__), "..", "..", "build", "ctp2")
)
GAME_BINARY = os.path.abspath(GAME_BINARY)

SOCKET_PATH = os.environ.get("CTP2_SMOKE_SOCKET", "/tmp/ctp2-smoke.sock")
TIMEOUT_INIT = 30


def send_cmd(cmd: str) -> dict:
    """Open a fresh connection, send one command, read response, close."""
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.settimeout(20)
    try:
        sock.connect(SOCKET_PATH)
        request = json.dumps({"cmd": cmd}) + "\n"
        sock.sendall(request.encode())

        buf = b""
        while b"\n" not in buf:
            chunk = sock.recv(4096)
            if not chunk:
                break
            buf += chunk

        line = buf.decode().strip().split("\n")[0]
        return json.loads(line)
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

    # Game expects to find data files (appstr.txt, etc.) in working directory
    game_cwd = os.environ.get("CTP2_CWD")
    if not game_cwd:
        # Walk up from binary location looking for appstr.txt
        candidate = os.path.dirname(os.path.dirname(GAME_BINARY))
        while candidate and candidate != "/":
            if os.path.exists(os.path.join(candidate, "appstr.txt")):
                game_cwd = candidate
                break
            candidate = os.path.dirname(candidate)
    print(f"[SMOKE] Launching game from cwd={game_cwd} ...")
    log_file = open("/tmp/ctp2-smoke-game.log", "w")
    proc = subprocess.Popen(
        [GAME_BINARY, "smoke-test"],
        stdout=log_file,
        stderr=subprocess.STDOUT,
        cwd=game_cwd,
    )

    try:
        wait_for_socket(SOCKET_PATH, TIMEOUT_INIT)
    except TimeoutError as e:
        print(f"[SMOKE] FAIL: {e}")
        proc.terminate()
        proc.wait()
        log_file.close()
        try:
            with open("/tmp/ctp2-smoke-game.log") as f:
                print("[SMOKE] Game log:\n" + f.read())
        except Exception:
            pass
        return 1

    print("[SMOKE] Game initialized, running scenario...")

    tests_passed = 0
    tests_failed = 0

    def run(cmd: str, expect_ok: bool = True):
        nonlocal tests_passed, tests_failed
        print(f"[SMOKE] >>> {cmd}")
        try:
            resp = send_cmd(cmd)
        except Exception as e:
            print(f"[SMOKE] FAIL: {cmd} -> {e}")
            tests_failed += 1
            return None

        print(f"[SMOKE] <<< {resp}")
        if expect_ok and resp.get("status") != "ok":
            print(f"[SMOKE] FAIL: {cmd} returned error")
            tests_failed += 1
            return resp

        tests_passed += 1
        return resp

    # Scenario: main menu -> new game -> start -> settle -> end turn -> quit
    run("new_game")
    run("start_game")
    time.sleep(5)  # let map generation and initial unit placement finish
    run("build_city")
    time.sleep(2)  # let settle animation/process complete
    run("end_turn")
    run("end_turn")
    run("quit", expect_ok=False)

    print("[SMOKE] Waiting for game to exit...")
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        proc.terminate()
        proc.wait()

    log_file.close()

    total = tests_passed + tests_failed
    print(f"[SMOKE] Results: {tests_passed}/{total} passed, {tests_failed} failed")
    return 0 if tests_failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
