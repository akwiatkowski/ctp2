#!/usr/bin/env python3
"""
CTP2 Smoke Test Harness with Recording and Replay

Runs the game with --smoke-test flag and sends commands via Unix socket.
Supports recording gameplay sessions with auto-save every N turns.

Usage:
    # Run default smoke test
    ./test/smoke_test.py

    # Record a session with auto-save every 20 turns
    ./test/smoke_test.py --record --autosave-interval 20

    # Replay a recorded session
    ./test/smoke_test.py --replay test/recordings/session_2026-05-12_143022.json

    # Long soak test: replay recording, auto-save every 50 turns
    ./test/smoke_test.py --replay recording.json --autosave-interval 50 --soak

A scenario is a JSON file with a list of steps. If no file is given,
a default smoke test runs: new_game -> start_game -> end_turn -> quit.

Recording format (JSON):
    {
        "session_id": "session_2026-05-12_143022",
        "start_time": "2026-05-12T14:30:22",
        "autosave_interval": 20,
        "autosave_dir": "test/recordings/session_2026-05-12_143022",
        "commands": [
            {"seq": 1, "turn": 1, "cmd": "build_city", "wait": 3000,
             "timestamp": "2026-05-12T14:30:25"},
            {"seq": 3, "turn": 1, "cmd": "advance_turns 1", "wait": 10000,
             "timestamp": "2026-05-12T14:30:35", "result_turns": "1->2"}
        ],
        "autosaves": [
            {"turn": 20, "path": "test/recordings/session_X/turn_020.sav"}
        ]
    }
"""

import argparse
import json
import os
import queue
import socket
import subprocess
import sys
import tempfile
import threading
import time
from datetime import datetime

# Configuration (may be overridden by --build-dir)
GAME_EXE = "./build/ctp2"
SOCKET_PATH = "/tmp/ctp2-smoke.sock"
DEFAULT_TIMEOUT = 30  # seconds per step
GAME_DIR = "."
RECORDINGS_DIR = "test/recordings"


def create_lldb_script():
    """Create a temporary lldb script that runs the game and captures crashes."""
    script = """
settings set target.load-script-from-symbol-file true
breakpoint set -n exit -o true -C "bt all"
run
bt all
thread backtrace all
quit
"""
    fd, path = tempfile.mkstemp(suffix=".lldb", prefix="ctp2-smoke-")
    os.write(fd, script.encode())
    os.close(fd)
    return path


def send_command(sock, cmd):
    """Send a JSON command to the game and return the response."""
    request = json.dumps({"cmd": cmd}) + "\n"
    try:
        sock.sendall(request.encode())
    except (BrokenPipeError, OSError):
        if cmd == "quit":
            return {"status": "ok", "cmd": cmd, "detail": "game_exited"}
        return {"status": "error", "cmd": cmd, "detail": "broken_pipe"}

    sock.settimeout(10)
    try:
        response = sock.recv(512).decode().strip()
        return json.loads(response)
    except socket.timeout:
        return {"status": "error", "cmd": cmd, "detail": "timeout"}
    except (BrokenPipeError, OSError):
        if cmd == "quit":
            return {"status": "ok", "cmd": cmd, "detail": "game_exited"}
        return {"status": "error", "cmd": cmd, "detail": "broken_pipe"}
    except json.JSONDecodeError:
        return {"status": "error", "cmd": cmd, "detail": "bad_json"}


def get_current_turn(sock):
    """Query the game for current turn number. Returns turn or -1 on error."""
    resp = send_command(sock, "turn_counter")
    detail = resp.get("detail", "")
    if "round=" in detail:
        try:
            return int(detail.split("round=")[1].split(",")[0])
        except (ValueError, IndexError):
            pass
    return -1


def advance_turns(sock, n, wait_ms=12000):
    """Advance N turns by sending end_turn and polling until each completes."""
    start_round = get_current_turn(sock)
    print(f"[HARNESS] Advancing {n} turns from round {start_round}")
    current_round = start_round
    for i in range(n):
        resp = send_command(sock, "end_turn")
        if resp.get("status") != "ok":
            print(f"[HARNESS] end_turn failed at step {i+1}/{n}: {resp}")
            break
        polled = 0
        while polled < 15:
            time.sleep(1)
            polled += 1
            new_round = get_current_turn(sock)
            if new_round > current_round:
                current_round = new_round
                print(f"[HARNESS] Turn advanced to round {current_round}")
                break
        else:
            print(f"[HARNESS] Warning: turn did not advance after 15s")

    print(f"[HARNESS] Advanced from round {start_round} to {current_round}")
    return {"status": "ok", "cmd": f"advance_turns {n}",
            "detail": f"{start_round}->{current_round}",
            "result_turns": f"{start_round}->{current_round}"}


def autosave_game(sock, turn, autosave_dir, session_id):
    """Auto-save the game at the given turn. Returns the save path."""
    os.makedirs(autosave_dir, exist_ok=True)
    filename = f"turn_{turn:03d}.sav"
    path = os.path.join(autosave_dir, filename)
    print(f"[HARNESS] Auto-saving turn {turn} to {path}")
    resp = send_command(sock, f"save_game {path}")
    if resp.get("status") == "ok":
        print(f"[HARNESS] Auto-save successful: {path}")
    else:
        print(f"[HARNESS] Auto-save failed: {resp}")
    return path, resp


def run_scenario(steps, record_file=None, autosave_interval=0,
                 autosave_dir=None, replay_mode=False):
    """
    Run a test scenario against the game.

    Args:
        steps: List of scenario steps (dicts with 'cmd' and optional 'wait')
        record_file: Path to write recording JSON (None = don't record)
        autosave_interval: Save every N turns (0 = disabled)
        autosave_dir: Directory for auto-saves
        replay_mode: True if replaying (validates turn numbers)
    """
    recording = {
        "session_id": datetime.now().strftime("session_%Y-%m-%d_%H%M%S"),
        "start_time": datetime.now().isoformat(),
        "autosave_interval": autosave_interval,
        "autosave_dir": autosave_dir,
        "commands": [],
        "autosaves": [],
    }

    if autosave_dir:
        os.makedirs(autosave_dir, exist_ok=True)

    # Clean up stale socket
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)

    lldb_script = create_lldb_script()

    cmd = [
        "lldb",
        "-s", lldb_script,
        "--",
        os.path.abspath(GAME_EXE),
        "--smoke-test"
    ]

    print(f"[HARNESS] Launching: {' '.join(cmd)}")
    print(f"[HARNESS] Working dir: {os.path.abspath(GAME_DIR)}")

    process = subprocess.Popen(
        cmd,
        cwd=GAME_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=0,
    )

    stdout_queue = queue.Queue()
    output_lines = []

    def reader_thread():
        for line in iter(process.stdout.readline, b""):
            decoded = line.decode("utf-8", errors="replace").rstrip()
            stdout_queue.put(decoded)
        stdout_queue.put(None)

    t = threading.Thread(target=reader_thread, daemon=True)
    t.start()

    startup_timeout = 120
    server_ready = False
    start = time.time()
    while time.time() - start < startup_timeout:
        try:
            line = stdout_queue.get(timeout=0.5)
            if line is None:
                break
            output_lines.append(line)
            print(f"[GAME] {line}")
            if "[SMOKE] Server listening" in line:
                server_ready = True
                print("[HARNESS] Game server is ready")
                break
        except queue.Empty:
            continue

    if not server_ready:
        print("[HARNESS] FAILED: Game did not start smoke test server")
        process.kill()
        process.wait()
        os.unlink(lldb_script)
        return False, output_lines

    def printer_thread():
        while True:
            try:
                line = stdout_queue.get(timeout=0.5)
                if line is None:
                    break
                output_lines.append(line)
                print(f"[GAME] {line}")
            except queue.Empty:
                if process.poll() is not None:
                    break
                continue

    threading.Thread(target=printer_thread, daemon=True).start()

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    connected = False
    for attempt in range(50):
        time.sleep(0.2)
        try:
            sock.connect(SOCKET_PATH)
            connected = True
            break
        except socket.error:
            continue

    if not connected:
        print("[HARNESS] FAILED: Could not connect to socket after retries")
        process.kill()
        process.wait()
        os.unlink(lldb_script)
        return False, output_lines

    print("[HARNESS] Connected to game socket")

    # Track current turn for auto-save
    last_autosave_turn = 0
    current_turn = get_current_turn(sock)
    if current_turn > 0:
        last_autosave_turn = (current_turn // autosave_interval) * autosave_interval

    results = []
    for i, step in enumerate(steps):
        cmd_str = step.get("cmd")
        wait_ms = step.get("wait", 1000)

        # Replay mode: validate turn number
        expected_turn = step.get("turn")
        if replay_mode and expected_turn is not None:
            actual_turn = get_current_turn(sock)
            if actual_turn != expected_turn:
                print(f"[HARNESS] WARNING: Turn mismatch at step {i+1}: "
                      f"expected {expected_turn}, actual {actual_turn}")

        print(f"[HARNESS] Step {i+1}/{len(steps)}: {cmd_str} (wait {wait_ms}ms)")

        # Record command start
        cmd_record = {
            "seq": i + 1,
            "turn": current_turn,
            "cmd": cmd_str,
            "wait": wait_ms,
            "timestamp": datetime.now().isoformat(),
        }

        # Handle advance_turns specially
        if cmd_str.startswith("advance_turns "):
            try:
                n = int(cmd_str.split()[1])
                response = advance_turns(sock, n, wait_ms)
                cmd_record["result_turns"] = response.get("result_turns", "")
            except (ValueError, IndexError):
                response = {"status": "error", "cmd": cmd_str, "detail": "bad_count"}
        elif cmd_str.startswith("load_game "):
            response = send_command(sock, cmd_str)
            if response.get("detail") == "bad_json":
                print(f"[HARNESS] load_game: response lost due to reinit, "
                      f"checking if game alive...")
                time.sleep(2)
                check = send_command(sock, "turn_counter")
                if check.get("status") == "ok":
                    response = {"status": "ok", "cmd": cmd_str, "detail": "loaded_ok"}
                else:
                    response = {"status": "error", "cmd": cmd_str, "detail": "load_failed"}
        else:
            response = send_command(sock, cmd_str)

        detail = response.get("detail", "")
        if detail:
            print(f"[HARNESS] Response: {response} (detail: {detail})")
        else:
            print(f"[HARNESS] Response: {response}")

        cmd_record["response"] = response
        recording["commands"].append(cmd_record)
        results.append({"step": i, "cmd": cmd_str, "response": response})

        if response.get("status") != "ok":
            print(f"[HARNESS] Step failed: {response}")

        time.sleep(wait_ms / 1000.0)

        # Update current turn after advance_turns
        if cmd_str.startswith("advance_turns "):
            current_turn = get_current_turn(sock)

        # Auto-save check
        if autosave_interval > 0 and current_turn > 0:
            next_autosave = last_autosave_turn + autosave_interval
            if current_turn >= next_autosave:
                if autosave_dir is None:
                    autosave_dir = os.path.join(RECORDINGS_DIR, recording["session_id"])
                    os.makedirs(autosave_dir, exist_ok=True)
                    recording["autosave_dir"] = autosave_dir
                save_path, save_resp = autosave_game(
                    sock, current_turn, autosave_dir, recording["session_id"]
                )
                recording["autosaves"].append({
                    "turn": current_turn,
                    "path": save_path,
                    "status": save_resp.get("status", "unknown"),
                })
                last_autosave_turn = current_turn

    # Give game a bit more time, then check if still alive
    time.sleep(1)
    exit_code = process.poll()

    if exit_code is None:
        print("[HARNESS] Scenario completed, game still running. Terminating...")
        process.terminate()
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            print("[HARNESS] Terminate timed out, force killing...")
            process.kill()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                print("[HARNESS] Kill timed out, using SIGKILL...")
                import signal
                os.kill(process.pid, signal.SIGKILL)
                process.wait()
        success = True
    else:
        print(f"[HARNESS] Game exited with code {exit_code}")
        success = (exit_code == 0)

    # Final cleanup
    try:
        if process.poll() is None:
            print("[HARNESS] Process still alive after cleanup, force killing...")
            process.kill()
            process.wait(timeout=2)
    except Exception:
        pass

    sock.close()
    os.unlink(lldb_script)

    # Write recording file
    if record_file:
        recording["end_time"] = datetime.now().isoformat()
        recording["success"] = success
        with open(record_file, "w") as f:
            json.dump(recording, f, indent=2)
        print(f"[HARNESS] Recording saved to {record_file}")

    return success, output_lines


def load_recording(path):
    """Load a recorded session from JSON."""
    with open(path) as f:
        data = json.load(f)
    steps = []
    for cmd in data.get("commands", []):
        step = {"cmd": cmd["cmd"], "wait": cmd.get("wait", 1000)}
        if "turn" in cmd:
            step["turn"] = cmd["turn"]
        steps.append(step)
    return steps, data


def main():
    parser = argparse.ArgumentParser(description="CTP2 Smoke Test Harness")
    parser.add_argument("scenario_file", nargs="?", help="JSON scenario file")
    parser.add_argument("--build-dir", default="build",
                        help="Meson build directory (default: build)")
    parser.add_argument("--record", action="store_true",
                        help="Record all commands to a JSON file")
    parser.add_argument("--record-file",
                        help="Path for recording JSON (default: auto-generated)")
    parser.add_argument("--autosave-interval", type=int, default=0,
                        help="Auto-save every N turns (0 = disabled)")
    parser.add_argument("--autosave-dir",
                        help="Directory for auto-saves (default: auto-generated)")
    parser.add_argument("--replay",
                        help="Replay a recorded session JSON file")
    parser.add_argument("--soak", action="store_true",
                        help="Soak test mode: replay recording in a loop")
    parser.add_argument("--soak-iterations", type=int, default=10,
                        help="Number of iterations for soak test (default: 10)")
    args = parser.parse_args()

    global GAME_EXE
    GAME_EXE = f"./{args.build_dir}/ctp2"

    # Determine mode
    replay_mode = False
    record_file = None
    autosave_interval = args.autosave_interval
    autosave_dir = args.autosave_dir

    if args.replay:
        # Replay mode
        print(f"[HARNESS] Replaying session from {args.replay}")
        steps, recording_data = load_recording(args.replay)
        replay_mode = True
        # Use autosave settings from recording if not overridden
        if autosave_interval == 0:
            autosave_interval = recording_data.get("autosave_interval", 0)
        if autosave_dir is None:
            autosave_dir = recording_data.get("autosave_dir")
    elif args.scenario_file:
        with open(args.scenario_file) as f:
            scenario = json.load(f)
        steps = scenario.get("steps", [])
        print(f"[HARNESS] Loaded scenario from {args.scenario_file}: {len(steps)} steps")
    else:
        # Default smoke test
        steps = [
            {"cmd": "new_game", "wait": 1000},
            {"cmd": "start_game", "wait": 5000},
            {"cmd": "build_city", "wait": 3000},
            {"cmd": "turn_counter", "wait": 300},
            {"cmd": "list_visible_units", "wait": 300},
            {"cmd": "set_production 0 cheapest_military", "wait": 1000},
            {"cmd": "screenshot /tmp/ctp2-smoke-screenshot.bmp", "wait": 500},
            {"cmd": "save_game /tmp/ctp2-smoke-save.sav", "wait": 1000},
            {"cmd": "diplomacy_status 2", "wait": 300},
            {"cmd": "advance_turns 1", "wait": 10000},
            {"cmd": "turn_counter", "wait": 300},
            {"cmd": "load_game /tmp/ctp2-smoke-save.sav", "wait": 3000},
            {"cmd": "quit", "wait": 1000},
        ]
        print(f"[HARNESS] Using default scenario: {len(steps)} steps")

    # Setup recording
    if args.record:
        if args.record_file:
            record_file = args.record_file
        else:
            session_id = datetime.now().strftime("session_%Y-%m-%d_%H%M%S")
            record_file = os.path.join(RECORDINGS_DIR, f"{session_id}.json")
        print(f"[HARNESS] Recording enabled: {record_file}")

    # Setup auto-save directory
    if autosave_interval > 0 and autosave_dir is None:
        session_id = datetime.now().strftime("session_%Y-%m-%d_%H%M%S")
        autosave_dir = os.path.join(RECORDINGS_DIR, session_id)
        os.makedirs(autosave_dir, exist_ok=True)
        print(f"[HARNESS] Auto-save enabled every {autosave_interval} turns: {autosave_dir}")

    # Run scenario (or soak loop)
    all_success = True
    iterations = args.soak_iterations if args.soak else 1

    for iteration in range(iterations):
        if args.soak:
            print(f"\n[HARNESS] ===== SOAK TEST ITERATION {iteration + 1}/{iterations} =====")

        success, output = run_scenario(
            steps,
            record_file=record_file if iteration == 0 else None,
            autosave_interval=autosave_interval,
            autosave_dir=autosave_dir,
            replay_mode=replay_mode,
        )
        all_success = all_success and success

        if not success and not args.soak:
            break

    # Summary
    print("\n" + "=" * 60)
    if all_success:
        print("SMOKE TEST: PASS")
    else:
        print("SMOKE TEST: FAIL")
        print("\nGame output (last 50 lines):")
        for line in output[-50:]:
            print(line)
    print("=" * 60)

    sys.exit(0 if all_success else 1)


if __name__ == "__main__":
    main()
