#!/usr/bin/env python3
"""
CTP2 Play & Record - Interactive Gameplay Recorder

Launches the game with a Unix socket server and provides an interactive
command interface. All commands and game responses are logged to a JSON
file. Auto-saves every N turns for crash recovery.

Usage:
    # Record a gameplay session with auto-save every 20 turns
    python3 test/play_and_record.py --build-dir build-sanitized --autosave-interval 20

    # Replay commands from a recorded session (for debugging)
    python3 test/play_and_record.py --replay test/recordings/session_X.json

Commands (type in the interactive prompt):
    help              - Show available commands
    turn              - Show current turn
    end_turn          - End current turn
    build_city        - Build city with selected settler
    set_production N building_type
                      - Set city production
    move_unit city_idx dx dy
                      - Move unit from city
    set_research advance_name
                      - Set research target
    diplomacy_status player
                      - Check diplomatic relations
    save_game path    - Save game to path
    load_game path    - Load game from path
    screenshot path   - Take screenshot
    autosave          - Trigger auto-save now
    quit              - Exit game

Recording format:
    test/recordings/
    ├── session_2026-05-12_143022.json   # Command log + metadata
    └── session_2026-05-12_143022/       # Auto-saves
        ├── turn_001.sav
        ├── turn_020.sav
        └── turn_040.sav
"""

import argparse
import json
import os
import queue
import readline  # enables command history
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import time
from datetime import datetime

# Configuration
GAME_EXE = "./build/ctp2"
SOCKET_PATH = "/tmp/ctp2-smoke.sock"
GAME_DIR = "."
RECORDINGS_DIR = "test/recordings"


def create_lldb_script():
    """Create lldb script that runs the game and captures crashes."""
    script = """
settings set target.load-script-from-symbol-file true
breakpoint set -n exit -o true -C "bt all"
run
bt all
thread backtrace all
quit
"""
    fd, path = tempfile.mkstemp(suffix=".lldb", prefix="ctp2-play-")
    os.write(fd, script.encode())
    os.close(fd)
    return path


def send_command(sock, cmd):
    """Send a command to the game and return the response."""
    request = json.dumps({"cmd": cmd}) + "\n"
    try:
        sock.sendall(request.encode())
    except (BrokenPipeError, OSError):
        return {"status": "error", "cmd": cmd, "detail": "broken_pipe"}

    sock.settimeout(15)
    try:
        response = sock.recv(1024).decode().strip()
        if not response:
            return {"status": "error", "cmd": cmd, "detail": "empty_response"}
        return json.loads(response)
    except socket.timeout:
        return {"status": "error", "cmd": cmd, "detail": "timeout"}
    except json.JSONDecodeError:
        return {"status": "error", "cmd": cmd, "detail": "bad_json"}


def get_current_turn(sock):
    """Query current turn number from game."""
    resp = send_command(sock, "turn_counter")
    detail = resp.get("detail", "")
    if "round=" in detail:
        try:
            return int(detail.split("round=")[1].split(",")[0])
        except (ValueError, IndexError):
            pass
    return -1


def auto_save(sock, turn, autosave_dir, session_id):
    """Save game to auto-save directory."""
    os.makedirs(autosave_dir, exist_ok=True)
    path = os.path.join(autosave_dir, f"turn_{turn:03d}.sav")
    print(f"  [AUTO-SAVE] Saving turn {turn} to {path}")
    resp = send_command(sock, f"save_game {path}")
    if resp.get("status") == "ok":
        print(f"  [AUTO-SAVE] OK: {path}")
        return {"turn": turn, "path": path, "status": "ok"}
    else:
        print(f"  [AUTO-SAVE] FAILED: {resp}")
        return {"turn": turn, "path": path, "status": "failed", "error": resp}


def print_game_output(output_lines, game_dead):
    """Print any pending game output."""
    while True:
        try:
            line = output_lines.get_nowait()
            if line is None:
                game_dead[0] = True
                break
            if line.startswith("[SMOKE]"):
                continue  # Don't echo our own commands
            print(f"[GAME] {line}")
        except queue.Empty:
            break


def show_help():
    """Print available commands."""
    print("""
Available commands:
  help                          Show this help
  turn                          Show current game turn
  end_turn                      End current turn
  build_city                    Build city with selected settler
  set_production N type         Set city N production (cheapest_military, etc.)
  move_unit city dx dy          Move unit from city by offset
  set_research advance_name     Set research target (e.g. Agriculture)
  diplomacy_status player       Check relations with player N
  list_cities                   List all cities
  list_units                    List visible units
  save_game path                Manual save
  load_game path                Load saved game
  screenshot path.bmp           Take screenshot
  autosave                      Trigger auto-save now
  quit                          Exit game

  (Any other text is sent to the game as a raw smoke test command)
""")


def run_interactive(args):
    """Run the game in interactive recording mode."""
    # Setup recording
    session_id = datetime.now().strftime("session_%Y-%m-%d_%H%M%S")
    if args.record_file:
        record_path = args.record_file
    else:
        os.makedirs(RECORDINGS_DIR, exist_ok=True)
        record_path = os.path.join(RECORDINGS_DIR, f"{session_id}.json")

    autosave_dir = args.autosave_dir
    if autosave_dir is None and args.autosave_interval > 0:
        autosave_dir = os.path.join(RECORDINGS_DIR, session_id)

    recording = {
        "session_id": session_id,
        "start_time": datetime.now().isoformat(),
        "autosave_interval": args.autosave_interval,
        "autosave_dir": autosave_dir,
        "build_dir": args.build_dir,
        "commands": [],
        "autosaves": [],
        "crashes": [],
    }

    # Clean up stale socket
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)

    lldb_script = create_lldb_script()
    game_exe = os.path.abspath(f"./{args.build_dir}/ctp2")

    cmd = ["lldb", "-s", lldb_script, "--", game_exe, "--smoke-test"]
    print(f"[HARNESS] Launching: {' '.join(cmd)}")
    print(f"[HARNESS] Recording to: {record_path}")
    if autosave_dir:
        print(f"[HARNESS] Auto-save every {args.autosave_interval} turns: {autosave_dir}")

    process = subprocess.Popen(
        cmd,
        cwd=GAME_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=0,
        preexec_fn=os.setsid,  # Create new process group
    )

    # Reader thread
    stdout_queue = queue.Queue()
    output_lines = queue.Queue()
    game_dead = [False]

    def reader():
        for line in iter(process.stdout.readline, b""):
            decoded = line.decode("utf-8", errors="replace").rstrip()
            stdout_queue.put(decoded)
            output_lines.put(decoded)
        stdout_queue.put(None)
        output_lines.put(None)

    threading.Thread(target=reader, daemon=True).start()

    # Wait for game to be ready
    print("[HARNESS] Waiting for game to start...")
    ready = False
    for _ in range(240):  # 2 minute timeout
        try:
            line = stdout_queue.get(timeout=0.5)
            if line is None:
                break
            if "[SMOKE] Server listening" in line:
                ready = True
                break
        except queue.Empty:
            continue

    if not ready:
        print("[HARNESS] FAILED: Game did not start")
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        os.unlink(lldb_script)
        return False

    # Connect to socket
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    for attempt in range(50):
        try:
            time.sleep(0.2)
            sock.connect(SOCKET_PATH)
            break
        except socket.error:
            continue
    else:
        print("[HARNESS] FAILED: Could not connect to game socket")
        process.kill()
        process.wait()
        os.unlink(lldb_script)
        return False

    print("[HARNESS] Game ready! Type 'help' for commands.")
    print("=" * 60)

    # Track auto-save state
    last_autosave_turn = 0
    current_turn = get_current_turn(sock)
    if current_turn > 0:
        last_autosave_turn = (current_turn // args.autosave_interval) * args.autosave_interval

    seq = 0
    try:
        while not game_dead[0]:
            # Print any pending game output
            print_game_output(output_lines, game_dead)
            if game_dead[0]:
                break

            # Interactive prompt
            try:
                user_input = input("ctp2> ").strip()
            except EOFError:
                print("\n[HARNESS] EOF received, exiting...")
                break

            if not user_input:
                continue

            # Handle internal commands
            parts = user_input.split()
            base_cmd = parts[0]
            response = None
            cmd_str = user_input

            if base_cmd == "help":
                show_help()
                continue
            elif base_cmd == "turn":
                cmd_str = "turn_counter"
            elif base_cmd == "end_turn":
                cmd_str = "end_turn"
            elif base_cmd == "autosave":
                if autosave_dir and current_turn > 0:
                    save_info = auto_save(sock, current_turn, autosave_dir, session_id)
                    recording["autosaves"].append(save_info)
                else:
                    print("  [AUTO-SAVE] Disabled or no turn data")
                continue
            elif base_cmd == "list_cities":
                # List cities command
                print("  [HARNESS] Listing cities...")
                # Need to add list_cities to game - for now, just send raw
                pass

            # Record command
            seq += 1
            cmd_record = {
                "seq": seq,
                "turn": current_turn,
                "cmd": cmd_str,
                "timestamp": datetime.now().isoformat(),
                "user_input": user_input,
            }

            # Send to game
            if cmd_str == user_input:
                # Raw command
                response = send_command(sock, cmd_str)
            else:
                # Translated command
                response = send_command(sock, cmd_str)

            # Print response
            if response:
                status = response.get("status", "unknown")
                detail = response.get("detail", "")
                if status == "ok":
                    if detail:
                        print(f"  OK: {detail}")
                    else:
                        print(f"  OK")
                else:
                    print(f"  ERROR: {status} - {detail}")

            cmd_record["response"] = response
            recording["commands"].append(cmd_record)

            # Update turn after end_turn or advance_turns
            if base_cmd in ("end_turn", "advance_turns"):
                time.sleep(1)  # Give game time to process
                new_turn = get_current_turn(sock)
                if new_turn > current_turn:
                    print(f"  [TURN] Advanced to turn {new_turn}")
                    current_turn = new_turn

            # Auto-save check
            if args.autosave_interval > 0 and current_turn > 0 and autosave_dir:
                next_autosave = last_autosave_turn + args.autosave_interval
                if current_turn >= next_autosave:
                    save_info = auto_save(sock, current_turn, autosave_dir, session_id)
                    recording["autosaves"].append(save_info)
                    last_autosave_turn = current_turn

            # Check if game died
            if process.poll() is not None:
                print(f"\n[HARNESS] Game exited with code {process.poll()}")
                recording["crashes"].append({
                    "time": datetime.now().isoformat(),
                    "exit_code": process.poll(),
                    "last_command": cmd_str,
                })
                break

    except KeyboardInterrupt:
        print("\n[HARNESS] Interrupted by user")
    finally:
        # Final output
        print_game_output(output_lines, game_dead)

        # Save recording
        recording["end_time"] = datetime.now().isoformat()
        recording["final_turn"] = current_turn
        with open(record_path, "w") as f:
            json.dump(recording, f, indent=2)
        print(f"[HARNESS] Recording saved to {record_path}")

        # Cleanup
        if process.poll() is None:
            print("[HARNESS] Sending quit...")
            send_command(sock, "quit")
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("[HARNESS] Force killing...")
                os.killpg(os.getpgid(process.pid), signal.SIGKILL)
                process.wait()

        sock.close()
        os.unlink(lldb_script)

        # Print summary
        print("\n" + "=" * 60)
        print(f"Session: {session_id}")
        print(f"Commands: {seq}")
        print(f"Auto-saves: {len(recording['autosaves'])}")
        print(f"Final turn: {current_turn}")
        if recording["crashes"]:
            print(f"CRASHES: {len(recording['crashes'])}")
        print(f"Recording: {record_path}")
        print("=" * 60)

    return True


def run_replay(args):
    """Replay a recorded session for debugging."""
    with open(args.replay) as f:
        data = json.load(f)

    print(f"[HARNESS] Replaying {data['session_id']}")
    print(f"[HARNESS] Commands: {len(data['commands'])}")

    for cmd in data["commands"]:
        print(f"\n  {cmd['seq']:3d}. turn={cmd.get('turn', '?'):3d} cmd={cmd['cmd']}")
        if "response" in cmd:
            resp = cmd["response"]
            print(f"       -> {resp.get('status', '?')}: {resp.get('detail', '')}")

    return True


def main():
    parser = argparse.ArgumentParser(
        description="CTP2 Play & Record - Interactive Gameplay Recorder"
    )
    parser.add_argument("--build-dir", default="build",
                        help="Meson build directory (default: build)")
    parser.add_argument("--record-file",
                        help="Path for recording JSON")
    parser.add_argument("--autosave-interval", type=int, default=20,
                        help="Auto-save every N turns (default: 20)")
    parser.add_argument("--autosave-dir",
                        help="Directory for auto-saves")
    parser.add_argument("--replay",
                        help="Replay a recorded session JSON")
    args = parser.parse_args()

    if args.replay:
        run_replay(args)
    else:
        run_interactive(args)


if __name__ == "__main__":
    main()
