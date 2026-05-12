#!/usr/bin/env python3
"""
CTP2 Manual Play Session Wrapper

Runs the game with sanitizers and crash capture.
Auto-saves periodically in background while you play manually.

Usage:
    python3 test/manual_play.py --autosave-interval 30

Features:
    - Game window opens for manual play
    - Background auto-save every N turns via smoke test socket  
    - ASan/UBSan crash capture with full backtrace
    - Saves crash reports to test/crashes/
    - Saves auto-saves to test/manual_saves/

If the game crashes:
    1. ASan/UBSan output is printed to terminal
    2. Backtrace is saved to test/crashes/crash_TIMESTAMP.txt
    3. Last auto-save is in test/manual_saves/
    4. Share the crash report for debugging
"""

import argparse
import json
import os
import socket
import subprocess
import sys
import tempfile
import threading
import time
from datetime import datetime
from pathlib import Path

GAME_EXE = "./build-sanitized/ctp2"
SOCKET_PATH = "/tmp/ctp2-manual.sock"
SAVE_DIR = "test/manual_saves"
CRASH_DIR = "test/crashes"


def ensure_dir(path):
    Path(path).mkdir(parents=True, exist_ok=True)


def cleanup_socket():
    try:
        os.unlink(SOCKET_PATH)
    except FileNotFoundError:
        pass


def wait_for_socket(timeout=120):
    """Wait for game to create socket."""
    start = time.time()
    while time.time() - start < timeout:
        if os.path.exists(SOCKET_PATH):
            return True
        time.sleep(0.1)
    return False


def connect_socket():
    """Connect to game socket."""
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.settimeout(5)
    try:
        sock.connect(SOCKET_PATH)
        return sock
    except (socket.error, OSError):
        return None


def send_command(sock, cmd):
    """Send command and return response."""
    try:
        sock.sendall(cmd.encode())
    except (BrokenPipeError, OSError):
        return None

    sock.settimeout(10)
    try:
        data = sock.recv(512).decode().strip()
        try:
            return json.loads(data)
        except json.JSONDecodeError:
            return {"raw": data}
    except socket.timeout:
        return None
    except (BrokenPipeError, OSError):
        return None


def get_current_turn(sock):
    """Get current turn number."""
    resp = send_command(sock, "turn_counter")
    if resp and "detail" in resp:
        detail = resp["detail"]
        if "round=" in detail:
            try:
                return int(detail.split("round=")[1].split(",")[0])
            except (ValueError, IndexError):
                pass
    return -1


def auto_save_worker(sock_ref, stop_event, autosave_interval):
    """Background thread: auto-save every N turns."""
    ensure_dir(SAVE_DIR)
    last_turn = -1
    turns_since_save = 0
    last_save_path = None

    print(f"[AutoSave] Started (every {autosave_interval} turns)")

    while not stop_event.is_set():
        time.sleep(5)  # Check every 5 seconds

        sock = sock_ref[0]
        if not sock:
            continue

        turn = get_current_turn(sock)
        if turn < 0:
            continue

        if turn != last_turn:
            last_turn = turn
            turns_since_save += 1

            if turns_since_save >= autosave_interval:
                save_path = f"{SAVE_DIR}/turn_{turn:04d}.sav"
                try:
                    resp = send_command(sock, f"save_game {save_path}")
                    if resp and resp.get("status") == "ok":
                        last_save_path = save_path
                        turns_since_save = 0
                        print(f"[AutoSave] Saved turn {turn} -> {save_path}")
                except Exception as e:
                    print(f"[AutoSave] Error: {e}")

    print(f"[AutoSave] Stopped. Last save: {last_save_path or 'None'}")


def run_manual_session(build_dir, autosave_interval):
    """Run game for manual play with auto-save and crash capture."""
    ensure_dir(SAVE_DIR)
    ensure_dir(CRASH_DIR)

    exe = f"./{build_dir}/ctp2"
    if not os.path.exists(exe):
        print(f"Error: {exe} not found.")
        print("Build first: make setup-sanitized && make build-sanitized")
        return 1

    session_time = datetime.now().strftime("%Y%m%d_%H%M%S")
    crash_file = f"{CRASH_DIR}/crash_{session_time}.txt"
    log_file = f"{CRASH_DIR}/session_{session_time}.log"

    cleanup_socket()

    # Launch game with smoke test enabled
    env = os.environ.copy()
    # Keep SDL video for manual play (no dummy driver)

    print(f"Starting game: {exe}")
    print(f"Socket: {SOCKET_PATH}")
    print(f"Auto-save: every {autosave_interval} turns -> {SAVE_DIR}")
    print(f"Crash reports: {CRASH_DIR}")
    print("-" * 60)
    print("PLAY MANUALLY - The game window should open shortly.")
    print("Auto-save runs in background. Save manually too!")
    print("Press Ctrl+C to stop (game will close).")
    print("-" * 60)

    proc = subprocess.Popen(
        [exe, "--smoke-test", "--smoke-socket", SOCKET_PATH],
        stdout=open(log_file, "w"),
        stderr=subprocess.STDOUT,
        env=env,
        cwd=os.getcwd()
    )

    # Wait for socket
    if not wait_for_socket(120):
        print("ERROR: Game didn't create socket within 120s")
        proc.kill()
        return 1

    sock = connect_socket()
    if not sock:
        print("ERROR: Could not connect to game socket")
        proc.kill()
        return 1

    # Start auto-save thread
    stop_event = threading.Event()
    sock_ref = [sock]
    save_thread = threading.Thread(
        target=auto_save_worker,
        args=(sock_ref, stop_event, autosave_interval),
        daemon=True
    )
    save_thread.start()

    try:
        # Monitor game process
        while proc.poll() is None:
            time.sleep(1)

        # Game exited
        exit_code = proc.poll()
        print(f"\nGame exited with code {exit_code}")

        if exit_code != 0:
            print("=" * 60)
            print("CRASH OR ERROR DETECTED!")
            print("=" * 60)

            # Show last lines of log (ASan/UBSan output)
            if os.path.exists(log_file):
                with open(log_file) as f:
                    lines = f.readlines()
                    # Show last 100 lines
                    for line in lines[-100:]:
                        print(line.rstrip())

                # Also save to crash file
                with open(log_file) as src, open(crash_file, "w") as dst:
                    dst.write(f"Crash time: {datetime.now().isoformat()}\n")
                    dst.write(f"Exit code: {exit_code}\n")
                    dst.write(f"Auto-save dir: {SAVE_DIR}\n")
                    dst.write("=" * 60 + "\n")
                    dst.write("".join(lines))

                print(f"\nFull crash report saved to: {crash_file}")

            # List available saves
            saves = sorted(Path(SAVE_DIR).glob("turn_*.sav"))
            if saves:
                print(f"\nAvailable saves ({len(saves)}):")
                for s in saves[-5:]:  # Show last 5
                    print(f"  {s.name}")

            print("\nTo debug: share the crash report and I'll fix it!")

    except KeyboardInterrupt:
        print("\nStopping game...")
        try:
            send_command(sock, "quit")
        except:
            pass
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except:
            proc.kill()

    finally:
        stop_event.set()
        save_thread.join(timeout=5)
        try:
            sock.close()
        except:
            pass
        cleanup_socket()

    return 0


def main():
    parser = argparse.ArgumentParser(description="CTP2 Manual Play Session")
    parser.add_argument("--build-dir", default="build-sanitized",
                        help="Build directory (default: build-sanitized)")
    parser.add_argument("--autosave-interval", type=int, default=30,
                        help="Auto-save every N turns (default: 30)")
    args = parser.parse_args()

    return run_manual_session(args.build_dir, args.autosave_interval)


if __name__ == "__main__":
    sys.exit(main())
