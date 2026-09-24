#!/usr/bin/env python3
"""Record a visible human play session for later offscreen regression work."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import secrets
import shutil
import socket
import subprocess
import sys
import time
from datetime import datetime, timezone


def atomic_json(path, value):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")
    temporary.replace(path)


def git(root, *args):
    result = subprocess.run(["mise", "exec", "--", "git", *args], cwd=root,
                            text=True, capture_output=True, check=True)
    return result.stdout.strip()


def profile_source(root):
    explicit = os.environ.get("CTP2_PROFILE")
    if explicit:
        return Path(explicit).expanduser()
    home = Path(os.environ.get("CTP2_HOME", Path.home() / ".ctp2")).expanduser()
    candidate = home / "userprofile.txt"
    return candidate if candidate.exists() else root / "ctp2_code/test/testprofile.txt"


def asset_fingerprint():
    home = Path(os.environ.get("CTP2_HOME", Path.home() / ".ctp2")).expanduser()
    current = home / "assets/current"
    if current.is_symlink():
        return current.resolve().name
    pointer = home / "assets/current.txt"
    if pointer.exists():
        return pointer.read_text().strip()
    return None


class ControlClient:
    def __init__(self, path, timeout=5):
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.settimeout(timeout)
        self.socket.connect(str(path))
        self.buffer = b""

    def close(self):
        self.socket.close()

    def command(self, line):
        self.socket.sendall((json.dumps({"cmd": line}) + "\n").encode())
        while b"\n" not in self.buffer:
            chunk = self.socket.recv(65536)
            if not chunk:
                raise ConnectionError(f"control socket closed during {line!r}")
            self.buffer += chunk
        raw, self.buffer = self.buffer.split(b"\n", 1)
        return json.loads(raw)


def wait_for_control(path, process, timeout=120):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if path.exists():
            try:
                return ControlClient(path)
            except OSError:
                pass
        if process.poll() is not None:
            raise RuntimeError(f"game exited before recorder connected: {process.returncode}")
        time.sleep(0.1)
    raise TimeoutError(f"control socket did not appear: {path}")


def sample_process(pid, output):
    if platform.system() != "Darwin":
        return False
    try:
        subprocess.run(["/usr/bin/sample", str(pid), "2", "-file", str(output)],
                       timeout=10, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, check=False)
        return output.exists()
    except (OSError, subprocess.TimeoutExpired):
        return False


def rpc_ok(client, verb, *args):
    line = verb if not args else verb + " " + " ".join(map(str, args))
    response = client.command(line)
    if response.get("status") != "ok":
        raise RuntimeError(f"{line}: {response}")
    return response.get("result", {})


def write_monitor(journal, event, **data):
    journal.write(json.dumps({"time": time.time(), "event": event, **data}) + "\n")
    journal.flush()
    os.fsync(journal.fileno())


def capture_bundle(client, session, name, note, pid):
    directory = session / "bugs" / name
    directory.mkdir(parents=True, exist_ok=True)
    result = {"name": name, "note": note, "created_at": datetime.now(timezone.utc).isoformat()}
    try:
        result["responsive"] = True
        for verb in ("query_turn", "query_cities", "query_armies", "query_gpu_world"):
            result[verb] = client.command(verb)
        if result["query_turn"].get("status") == "ok":
            result["checkpoint"] = client.command(
                "save_game " + str(directory / "checkpoint.json"))
        result["frame"] = client.command(
            "screenshot_frame " + str(directory / "frame.bmp"))
    except Exception as error:
        result["responsive"] = False
        result["capture_error"] = repr(error)
        sample_process(pid, directory / "hang.sample.txt")
        last_frame = session / "last-frame.bmp"
        if last_frame.exists():
            shutil.copy2(last_frame, directory / "last-frame.bmp")
    atomic_json(directory / "bug.json", result)
    return result


def process_mark_request(client, session, pid):
    request_path = session / "mark-request.json"
    if not request_path.exists():
        return None
    try:
        request = json.loads(request_path.read_text())
    finally:
        request_path.unlink(missing_ok=True)
    return capture_bundle(client, session, request["id"], request.get("note", "reported bug"), pid)


def record(args):
    root = Path(__file__).resolve().parents[2]
    binary = (root / args.binary).resolve()
    commit = git(root, "rev-parse", "--short=8", "HEAD")
    dirty = bool(git(root, "status", "--porcelain"))
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    session_id = f"{timestamp}-{commit}-{secrets.token_hex(2)}"
    output_root = Path(args.output_root).resolve() if args.output_root else binary.parent / "playtests"
    session = output_root / session_id
    session.mkdir(parents=True)
    latest = session.parent / "latest"
    latest.unlink(missing_ok=True)
    latest.symlink_to(session.name)

    seed = secrets.randbelow(2_147_483_646) + 1
    source_profile = Path(args.profile_source) if args.profile_source else profile_source(root)
    profile = session / "profile.txt"
    shutil.copy2(source_profile, profile)
    profile_hash = hashlib.sha256(profile.read_bytes()).hexdigest()
    control = session / "control.sock"
    manifest = {
        "session": session_id,
        "created_at": datetime.now(timezone.utc).isoformat(),
        "seed": seed,
        "commit": commit,
        "dirty": dirty,
        "binary": str(binary),
        "profile_source": str(source_profile),
        "profile_sha256": profile_hash,
        "asset_fingerprint": asset_fingerprint(),
        "resumed_from": args.resume_from,
        "control_socket": str(control),
        "status": "starting",
    }
    atomic_json(session / "manifest.json", manifest)

    env = dict(os.environ, CTP2_PROFILE=str(profile), CTP2_SMOKE_SOCKET=str(control),
               CTP2_CAPTURE_FRAMES="1", CTP2_PLAYTEST_DIR=str(session))
    log = open(session / "game.log", "w")
    command = [str(binary), "--play-record", "--play-seed", str(seed), *args.game_args]
    process = subprocess.Popen(command, cwd=root, env=env, stdout=log,
                               stderr=subprocess.STDOUT)
    manifest.update(status="running", pid=process.pid, command=command)
    atomic_json(session / "manifest.json", manifest)
    print(f"Playtest session: {session}", flush=True)
    print(f"Seed: {seed}", flush=True)
    print("Play normally. Report a bug; the active session is build/playtests/latest.", flush=True)

    client = None
    round_seen = None
    hang_captured = False
    record_started = time.monotonic()
    try:
        client = wait_for_control(control, process)
        if args.smoke_start:
            def click_control(path):
                deadline = time.monotonic() + 30
                while time.monotonic() < deadline:
                    response = client.command("ui_control_bounds " + path)
                    bounds = response.get("result", {})
                    if (response.get("status") == "ok" and bounds.get("visible")
                            and bounds.get("enabled")):
                        x = bounds["x"] + bounds["width"] // 2
                        y = bounds["y"] + bounds["height"] // 2
                        for down in (0, 1, 0):
                            rpc_ok(client, "ui_pointer", x, y, down)
                        return
                    time.sleep(0.05)
                raise TimeoutError(f"control did not become clickable: {path}")
            click_control("InitPlayWindow.NewGameButton")
            rpc_ok(client, "ui_prepare_game", seed, 4)
            click_control("SPNewGameWindow.StartButton")
        with open(session / "monitor.jsonl", "a") as monitor:
            write_monitor(monitor, "connected", pid=process.pid)
            while process.poll() is None:
                if args.max_seconds and time.monotonic() - record_started >= args.max_seconds:
                    client.command("quit")
                    break
                try:
                    turn_response = client.command("query_turn")
                    if turn_response.get("status") == "ok":
                        turn = turn_response["result"]
                        current_round = turn["round"]
                        if current_round != round_seen:
                            round_seen = current_round
                            checkpoints = session / "checkpoints"
                            checkpoints.mkdir(exist_ok=True)
                            state = {
                                "turn": turn,
                                "cities": rpc_ok(client, "query_cities"),
                                "armies": rpc_ok(client, "query_armies"),
                            }
                            rpc_ok(client, "save_game", checkpoints / f"turn-{current_round:04d}.json")
                            try:
                                state["frame"] = rpc_ok(
                                    client, "screenshot_frame",
                                    checkpoints / f"turn-{current_round:04d}.bmp")
                            except RuntimeError:
                                pass
                            atomic_json(checkpoints / f"turn-{current_round:04d}-state.json", state)
                            write_monitor(monitor, "checkpoint", round=current_round)
                    try:
                        rpc_ok(client, "screenshot_frame", session / "last-frame.bmp")
                    except RuntimeError:
                        pass
                    marked = process_mark_request(client, session, process.pid)
                    if marked:
                        write_monitor(monitor, "bug_marked", result=marked)
                        print(f"Bug bundle: {session / 'bugs' / marked['name']}", flush=True)
                    time.sleep(2)
                except (socket.timeout, TimeoutError) as error:
                    if not hang_captured:
                        name = datetime.now(timezone.utc).strftime("hang-%Y%m%dT%H%M%SZ")
                        result = capture_bundle(client, session, name, "automatic watchdog timeout", process.pid)
                        write_monitor(monitor, "watchdog_timeout", error=repr(error), result=result)
                        print(f"Game stopped responding; diagnostics: {session / 'bugs' / name}", flush=True)
                        hang_captured = True
                    break
                except (ConnectionError, OSError) as error:
                    write_monitor(monitor, "control_disconnected", error=repr(error))
                    break
            process.wait()
    except KeyboardInterrupt:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    finally:
        if client:
            client.close()
        log.close()
        if process.poll() is None:
            process.wait()
        manifest.update(status="finished" if process.returncode == 0 else "failed",
                        exit_code=process.returncode,
                        finished_at=datetime.now(timezone.utc).isoformat())
        atomic_json(session / "manifest.json", manifest)
    if process.returncode != 0:
        raise SystemExit(process.returncode if process.returncode > 0 else 1)


def mark(args):
    root = Path(__file__).resolve().parents[2]
    latest = root / "build/playtests/latest"
    if not latest.exists():
        raise SystemExit("no active or previous playtest recording")
    session = latest.resolve()
    identifier = datetime.now(timezone.utc).strftime("bug-%Y%m%dT%H%M%SZ")
    request = {"id": identifier, "note": args.note, "created_at": datetime.now(timezone.utc).isoformat()}
    atomic_json(session / "mark-request.json", request)
    target = session / "bugs" / identifier / "bug.json"
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        if target.exists():
            print(f"Bug bundle: {target.parent}")
            return
        time.sleep(0.1)
    manifest = json.loads((session / "manifest.json").read_text())
    directory = target.parent
    directory.mkdir(parents=True, exist_ok=True)
    sample_process(manifest.get("pid", 0), directory / "hang.sample.txt")
    last_frame = session / "last-frame.bmp"
    if last_frame.exists():
        shutil.copy2(last_frame, directory / "last-frame.bmp")
    atomic_json(target, {**request, "responsive": False, "capture_error": "recorder did not answer mark request"})
    print(f"Unresponsive bug bundle: {directory}")

def resume(args):
    root = Path(__file__).resolve().parents[2]
    latest = root / "build/playtests/latest"
    if not latest.exists():
        raise SystemExit("no recorded playtest to resume")
    previous = latest.resolve()
    checkpoints = list((previous / "checkpoints").glob("turn-[0-9][0-9][0-9][0-9].json"))
    checkpoints.extend((previous / "bugs").glob("*/checkpoint.json"))
    if not checkpoints:
        raise SystemExit(f"no checkpoint in {previous}")
    checkpoint = max(checkpoints, key=lambda path: path.stat().st_mtime)
    options = argparse.Namespace(
        binary=args.binary,
        max_seconds=args.max_seconds,
        smoke_start=False,
        output_root=args.output_root,
        game_args=[f"-l{checkpoint}"],
        profile_source=str(previous / "profile.txt"),
        resume_from=str(checkpoint),
    )
    print(f"Resuming checkpoint: {checkpoint}", flush=True)
    record(options)



def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    recording = sub.add_parser("record")
    recording.add_argument("--max-seconds", type=float, default=0,
                           help=argparse.SUPPRESS)
    recording.add_argument("--binary", default="build/ctp2")
    recording.add_argument("--smoke-start", action="store_true",
                           help=argparse.SUPPRESS)
    recording.add_argument("--output-root", help=argparse.SUPPRESS)
    recording.add_argument("game_args", nargs=argparse.REMAINDER)
    recording.set_defaults(profile_source=None, resume_from=None)
    resuming = sub.add_parser("resume")
    resuming.add_argument("--binary", default="build/ctp2")
    resuming.add_argument("--max-seconds", type=float, default=0,
                          help=argparse.SUPPRESS)
    resuming.add_argument("--output-root", help=argparse.SUPPRESS)
    marking = sub.add_parser("mark")
    marking.add_argument("--note", default="reported bug")
    args = parser.parse_args()
    {"record": record, "resume": resume, "mark": mark}[args.command](args)


if __name__ == "__main__":
    main()
