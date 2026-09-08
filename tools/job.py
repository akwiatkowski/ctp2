#!/usr/bin/env python3
"""Manage local project jobs: start NAME COMMAND..., stop/status/logs NAME.

Example: mise exec -- python3 tools/job.py start scenarios mise exec -- meson test -C build --suite scenario
Approve the prefix `mise exec -- python3 tools/job.py` for subsequent job control.
Only jobs started here can be stopped. Metadata and logs live in build/jobs/.
"""

import argparse
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
JOBS = ROOT / "build" / "jobs"


def identity(pid):
    result = subprocess.run(
        ["ps", "-p", str(pid), "-o", "pgid=", "-o", "lstart=", "-o", "stat="],
        capture_output=True, text=True, check=False,
    )
    fields = result.stdout.split()
    if result.returncode or not fields or fields[-1].startswith("Z"):
        return None
    return " ".join(fields[:-1])


def running(job):
    return job["identity"] is not None and identity(job["pid"]) == job["identity"]


def process_tree(job):
    # Meson creates separate sessions for tests, so a single killpg misses them.
    result = subprocess.run(["ps", "-axo", "pid=,ppid=,pgid=,lstart=,stat="],
                            capture_output=True, text=True, check=True)
    rows = [line.split() for line in result.stdout.splitlines()]
    jobs = [job]
    for parent in jobs:
        for fields in rows:
            if int(fields[1]) == parent["pid"] and not fields[-1].startswith("Z"):
                jobs.append({"pid": int(fields[0]), "identity": " ".join(fields[2:-1])})
    return jobs


def start(name, command):
    JOBS.mkdir(parents=True, exist_ok=True)
    state = JOBS / f"{name}.json"
    if state.exists() and running(json.loads(state.read_text())):
        raise RuntimeError(f"{name} is already running")
    with (JOBS / f"{name}.log").open("wb") as log:
        process = subprocess.Popen(
            command, cwd=ROOT, stdin=subprocess.DEVNULL,
            stdout=log, stderr=subprocess.STDOUT, start_new_session=True,
        )
    job = {"pid": process.pid, "identity": identity(process.pid), "command": command}
    state.write_text(json.dumps(job, indent=2) + "\n")
    print(f"{name}: started PID {process.pid}; log: {JOBS / (name + '.log')}")
    return process


def stop(job):
    if not running(job):
        return
    jobs = process_tree(job)
    for sig in (signal.SIGTERM, signal.SIGKILL):
        for target in jobs:
            if running(target):
                try:
                    os.kill(target["pid"], sig)
                except ProcessLookupError:
                    pass
        if sig == signal.SIGTERM:
            deadline = time.monotonic() + 5
            while any(running(target) for target in jobs) and time.monotonic() < deadline:
                time.sleep(0.1)


def self_test():
    global JOBS
    with tempfile.TemporaryDirectory(prefix="ctp2-job-test-") as directory:
        JOBS = Path(directory)
        child = "import signal,time; signal.signal(signal.SIGTERM, signal.SIG_IGN); print('ready', flush=True); time.sleep(60)"
        parent = f"import subprocess,sys,time; subprocess.Popen([sys.executable, '-c', {child!r}], start_new_session=True); time.sleep(60)"
        process = start("test", [sys.executable, "-c", parent])
        job = json.loads((JOBS / "test.json").read_text())
        try:
            deadline = time.monotonic() + 5
            while "ready" not in (JOBS / "test.log").read_text():
                assert time.monotonic() < deadline, "child failed to start"
                time.sleep(0.05)
            assert running(job)
            stale = dict(job, identity="stale process identity")
            stop(stale)
            assert running(job), "a stale PID record must not stop a process"
            try:
                start("test", [sys.executable, "-c", "pass"])
            except RuntimeError:
                pass
            else:
                raise AssertionError("duplicate active job was accepted")
            children = process_tree(job)
            assert len(children) == 2
            stop(job)
            process.wait(timeout=10)
            assert not running(job)
            deadline = time.monotonic() + 5
            while any(running(target) for target in children):
                assert time.monotonic() < deadline, "child survived group shutdown"
                time.sleep(0.05)
            stop(job)  # Repeated stops are harmless.
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
    print("PASS: start, duplicate rejection, stale-PID protection and forced shutdown across child sessions")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["start", "stop", "status", "logs", "self-test"])
    parser.add_argument("name", nargs="?")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.action == "self-test":
        self_test()
        return
    if not args.name or not re.fullmatch(r"[A-Za-z0-9_-]+", args.name):
        parser.error("NAME must contain only letters, digits, underscores or hyphens")
    if args.action == "start":
        if not args.command:
            parser.error("start requires a command")
        start(args.name, args.command)
        return
    if args.command:
        parser.error("only start accepts a command")
    if args.action == "logs":
        print((JOBS / f"{args.name}.log").read_text(errors="replace"), end="")
        return
    job = json.loads((JOBS / f"{args.name}.json").read_text())
    if args.action == "stop":
        stop(job)
    print(f"{args.name}: {'running' if running(job) else 'not running'} (PID {job['pid']})")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError) as error:
        if "self-test" in sys.argv:
            raise
        sys.exit(str(error))
