#!/usr/bin/env python3
"""
CTP2 Smoke Test Harness

Runs the game with --smoke-test flag and sends commands via Unix socket.
The game is wrapped in lldb so crashes produce backtraces for LLM analysis.

Usage:
    ./test/smoke_test.py [scenario_file]

A scenario is a JSON file with a list of steps. If no file is given,
a default smoke test runs: new_game -> start_game -> end_turn -> quit.
"""

import json
import os
import queue
import socket
import subprocess
import sys
import tempfile
import threading
import time

# Configuration
GAME_EXE = "./build/ctp2"
SOCKET_PATH = "/tmp/ctp2-smoke.sock"
DEFAULT_TIMEOUT = 30  # seconds per step
GAME_DIR = "."


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
        # Game exited before/during send (expected for quit)
        if cmd == "quit":
            return {"status": "ok", "cmd": cmd, "detail": "game_exited"}
        return {"status": "error", "cmd": cmd, "detail": "broken_pipe"}

    # Read response (simple line-based)
    sock.settimeout(10)
    try:
        response = sock.recv(512).decode().strip()
        return json.loads(response)
    except socket.timeout:
        return {"status": "error", "cmd": cmd, "detail": "timeout"}
    except (BrokenPipeError, OSError):
        # Game exited before response (expected for quit)
        if cmd == "quit":
            return {"status": "ok", "cmd": cmd, "detail": "game_exited"}
        return {"status": "error", "cmd": cmd, "detail": "broken_pipe"}
    except json.JSONDecodeError:
        return {"status": "error", "cmd": cmd, "detail": "bad_json"}


def run_scenario(steps):
    """Run a test scenario against the game."""
    # Clean up stale socket
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)

    # Create lldb script
    lldb_script = create_lldb_script()

    # Launch game under lldb
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

    # Shared queue for stdout lines
    stdout_queue = queue.Queue()
    output_lines = []

    def reader_thread():
        """Read stdout and put lines into the queue."""
        for line in iter(process.stdout.readline, b""):
            decoded = line.decode("utf-8", errors="replace").rstrip()
            stdout_queue.put(decoded)
        stdout_queue.put(None)  # sentinel

    t = threading.Thread(target=reader_thread, daemon=True)
    t.start()

    # Wait for server to be ready (read from queue)
    server_ready = False
    start = time.time()
    while time.time() - start < 30:
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

    # Start background printer for remaining output
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

    # Connect to socket with retries
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    connected = False
    for attempt in range(10):
        time.sleep(0.5)
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

    # Execute scenario steps
    results = []
    for i, step in enumerate(steps):
        cmd = step.get("cmd")
        wait_ms = step.get("wait", 2000)

        print(f"[HARNESS] Step {i+1}/{len(steps)}: {cmd} (wait {wait_ms}ms)")

        response = send_command(sock, cmd)
        print(f"[HARNESS] Response: {response}")
        results.append({"step": i, "cmd": cmd, "response": response})

        if response.get("status") != "ok":
            print(f"[HARNESS] Step failed: {response}")
            # Continue anyway to see if game crashes

        # Wait for game to process
        time.sleep(wait_ms / 1000.0)

    # Give game a bit more time, then check if still alive
    time.sleep(2)
    exit_code = process.poll()

    if exit_code is None:
        print("[HARNESS] Scenario completed, game still running. Terminating...")
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        success = True
    else:
        print(f"[HARNESS] Game exited with code {exit_code}")
        success = (exit_code == 0)

    sock.close()
    os.unlink(lldb_script)

    return success, output_lines


def main():
    # Default smoke test scenario
    default_scenario = [
        {"cmd": "new_game", "wait": 2000},
        {"cmd": "start_game", "wait": 10000},  # Game needs time to generate map
        {"cmd": "build_city", "wait": 5000},   # Build city with starting settler
        {"cmd": "set_production 0 cheapest_military", "wait": 2000},
        {"cmd": "enable_governor all growth", "wait": 2000},
        {"cmd": "end_turn", "wait": 10000},    # Turn processing
        {"cmd": "quit", "wait": 2000},         # Clean exit
    ]

    scenario_file = sys.argv[1] if len(sys.argv) > 1 else None

    if scenario_file:
        with open(scenario_file) as f:
            scenario = json.load(f)
        steps = scenario.get("steps", [])
        print(f"[HARNESS] Loaded scenario from {scenario_file}: {len(steps)} steps")
    else:
        steps = default_scenario
        print(f"[HARNESS] Using default scenario: {len(steps)} steps")

    success, output = run_scenario(steps)

    # Summary
    print("\n" + "=" * 60)
    if success:
        print("SMOKE TEST: PASS")
    else:
        print("SMOKE TEST: FAIL")
        print("\nGame output (last 50 lines):")
        for line in output[-50:]:
            print(line)
    print("=" * 60)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
