#!/usr/bin/env python3
"""
CTP2 Long-Running Game Session

Runs the game for hours with auto-save every N turns.
If the game crashes, captures stack trace via lldb and restarts from last save.

Usage:
    python3 test/long_runner.py --build-dir build --autosave-interval 50 --max-turns 1000

Features:
    - Auto-save every N turns
    - Automatic turn advancement (AI vs AI mode)
    - Crash detection and automatic restart from last save
    - Stack trace capture on crash
    - Progress logging
"""

import argparse
import json
import os
import signal
import socket
import subprocess
import sys
import tempfile
import time
from datetime import datetime
from pathlib import Path

GAME_EXE = "./build/ctp2"
SOCKET_PATH = "/tmp/ctp2-longrun.sock"
SAVE_DIR = "test/longrun_saves"
CRASH_DIR = "test/crashes"


def ensure_dir(path):
    Path(path).mkdir(parents=True, exist_ok=True)


def cleanup_socket():
    try:
        os.unlink(SOCKET_PATH)
    except FileNotFoundError:
        pass


def create_lldb_script(save_path):
    """Create lldb script that saves core dump on crash."""
    return f"""
settings set target.run-args --smoke-test --smoke-socket {SOCKET_PATH}
breakpoint set --name abort
breakpoint set --name __assert_rtn
command script import lldb
import os

def save_backtrace(frame, bp_loc, dict):
    thread = frame.GetThread()
    process = thread.GetProcess()
    print("\\n[CRASH] Game crashed! Saving backtrace...")
    for f in thread:
        print(f"  {{f.GetDisplayFunctionName()}} at {{f.GetLineEntry().GetFileSpec()}}:{{f.GetLineEntry().GetLine()}}")
    
    # Save backtrace to file
    with open("{save_path}", "w") as f:
        f.write(f"Crash time: {{datetime.now().isoformat()}}\\n")
        f.write(f"PID: {{process.GetProcessID()}}\\n\\n")
        for fr in thread:
            f.write(f"{{fr.GetDisplayFunctionName()}} at {{fr.GetLineEntry().GetFileSpec()}}:{{fr.GetLineEntry().GetLine()}}\\n")
    
    process.Continue()
    return False

# Set up crash handlers
for bp in ["abort", "__assert_rtn", "__assert_fail", "__stack_chk_fail", "SIGSEGV", "SIGBUS", "SIGILL", "SIGFPE"]:
    target = lldb.debugger.GetSelectedTarget()
    if target:
        b = target.BreakpointCreateByName(bp)
        if b.GetNumLocations() > 0:
            b.SetScriptCallbackFunction("save_backtrace")

continue
"""


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


def wait_for_socket(timeout=60):
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


def save_game(sock, path):
    """Save game to path."""
    resp = send_command(sock, f"save_game {path}")
    return resp and resp.get("status") == "ok"


def load_game(sock, path):
    """Load game from path."""
    resp = send_command(sock, f"load_game {path}")
    return resp and resp.get("status") == "ok"


def advance_turn(sock):
    """End current turn."""
    resp = send_command(sock, "end_turn")
    return resp and resp.get("status") == "ok"


def run_game_session(build_dir, autosave_interval, max_turns, no_lldb):
    """Run a long game session with auto-save."""
    ensure_dir(SAVE_DIR)
    ensure_dir(CRASH_DIR)

    exe = f"./{build_dir}/ctp2"
    if not os.path.exists(exe):
        print(f"Error: {exe} not found. Build first with: make build")
        return 1

    session_start = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = f"{CRASH_DIR}/session_{session_start}.log"
    crash_file = f"{CRASH_DIR}/crash_{session_start}.txt"

    # Track state
    last_save = None
    current_turn = 0
    start_time = time.time()

    while current_turn < max_turns:
        cleanup_socket()

        # Prepare game launch
        env = os.environ.copy()
        env["SDL_VIDEODRIVER"] = "dummy"  # Headless mode for long runs

        if no_lldb:
            # Direct launch
            proc = subprocess.Popen(
                [exe, "--smoke-test", "--smoke-socket", SOCKET_PATH],
                stdout=open(log_file, "a"),
                stderr=subprocess.STDOUT,
                env=env,
                cwd=os.getcwd()
            )
        else:
            # Launch with lldb for crash capture
            lldb_script = tempfile.NamedTemporaryFile(mode="w", suffix=".py", delete=False)
            lldb_script.write(create_lldb_script(crash_file))
            lldb_script.close()

            proc = subprocess.Popen(
                ["lldb", "-b", "-s", lldb_script.name, "--", exe,
                 "--smoke-test", "--smoke-socket", SOCKET_PATH],
                stdout=open(log_file, "a"),
                stderr=subprocess.STDOUT,
                env=env,
                cwd=os.getcwd()
            )

        print(f"[{datetime.now().strftime('%H:%M:%S')}] Game started (PID {proc.pid})")

        # Wait for socket
        if not wait_for_socket(120):
            print("Timeout waiting for game socket")
            proc.kill()
            proc.wait()
            break

        sock = connect_socket()
        if not sock:
            print("Failed to connect to game socket")
            proc.kill()
            proc.wait()
            break

        # If we have a save, load it; otherwise start new game
        if last_save and os.path.exists(last_save):
            print(f"Loading save: {last_save}")
            if not load_game(sock, last_save):
                print("Failed to load save, starting new game")
                last_save = None
                send_command(sock, "new_game")
                time.sleep(2)
                send_command(sock, "start_game")
                time.sleep(5)
        else:
            print("Starting new game...")
            send_command(sock, "new_game")
            time.sleep(2)
            send_command(sock, "start_game")
            time.sleep(5)

        # Main game loop
        turns_since_save = 0
        while proc.poll() is None and current_turn < max_turns:
            # Get current turn
            turn = get_current_turn(sock)
            if turn > 0:
                if turn != current_turn:
                    current_turn = turn
                    turns_since_save += 1
                    elapsed = time.time() - start_time
                    print(f"[{datetime.now().strftime('%H:%M:%S')}] Turn {current_turn} "
                          f"(elapsed: {elapsed/60:.1f}min, saves: {turns_since_save}/{autosave_interval})")

                # Auto-save
                if turns_since_save >= autosave_interval:
                    save_path = f"{SAVE_DIR}/turn_{current_turn:04d}.sav"
                    if save_game(sock, save_path):
                        last_save = save_path
                        turns_since_save = 0
                        print(f"  Auto-saved to {save_path}")

            # Advance turn
            if not advance_turn(sock):
                print("Failed to advance turn, may have crashed")
                break

            # Wait for turn to complete (poll every second)
            for _ in range(30):  # Max 30 seconds per turn
                if proc.poll() is not None:
                    break
                time.sleep(1)
                # Check if turn advanced
                new_turn = get_current_turn(sock)
                if new_turn > current_turn:
                    break

        # Check if game crashed
        if proc.poll() is not None:
            exit_code = proc.poll()
            print(f"[{datetime.now().strftime('%H:%M:%S')}] Game exited with code {exit_code}")

            if os.path.exists(crash_file):
                print(f"CRASH DETECTED! Backtrace saved to {crash_file}")
                with open(crash_file) as f:
                    print(f.read())

                # Ask user what to do
                if last_save:
                    print(f"\nRestarting from last save: {last_save}")
                    time.sleep(2)
                    continue  # Restart loop with load
                else:
                    print("No save available to restart from")
                    break
            else:
                print("Game exited cleanly (no crash detected)")
                break

        # Clean up
        try:
            send_command(sock, "quit")
        except:
            pass
        try:
            sock.close()
        except:
            pass

        proc.wait(timeout=10)

    total_time = time.time() - start_time
    print(f"\n{'='*60}")
    print(f"Session complete!")
    print(f"Total turns: {current_turn}")
    print(f"Total time: {total_time/60:.1f} minutes ({total_time/3600:.2f} hours)")
    print(f"Last save: {last_save or 'None'}")
    print(f"Log file: {log_file}")
    print(f"{'='*60}")

    return 0


def main():
    parser = argparse.ArgumentParser(description="CTP2 Long-Running Game Session")
    parser.add_argument("--build-dir", default="build", help="Build directory (default: build)")
    parser.add_argument("--autosave-interval", type=int, default=50,
                        help="Auto-save every N turns (default: 50)")
    parser.add_argument("--max-turns", type=int, default=10000,
                        help="Maximum turns to run (default: 10000)")
    parser.add_argument("--no-lldb", action="store_true",
                        help="Don't use lldb (faster but no crash backtrace)")
    args = parser.parse_args()

    try:
        return run_game_session(args.build_dir, args.autosave_interval, args.max_turns, args.no_lldb)
    except KeyboardInterrupt:
        print("\nInterrupted by user")
        return 0


if __name__ == "__main__":
    sys.exit(main())
