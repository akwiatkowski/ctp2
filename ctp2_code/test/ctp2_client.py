#!/usr/bin/env python3
"""
Reusable client for the CTP2 command/query API.

Both game binaries expose the same newline-delimited JSON protocol over a Unix
socket (/tmp/ctp2-smoke.sock):

    request :  {"cmd": "<verb> <args...>"}\\n
    response:  {"status": "ok"|"error", "cmd": "...", "result": {...}|"detail": "..."}\\n

The ONLY difference between the two builds is how the game is created, which is
why this client is build-agnostic for every command except game setup:

    * mode="headless"  ->  ctp2_headless --serve   (game created in-process by
                           start_game, synchronously)
    * mode="ui"        ->  ctp2 --smoke-test        (start_game drives the real
                           menu screens; the game loads asynchronously over
                           several frames, so callers must wait_game_loaded())

A test written against this client therefore runs identically on both binaries
— that parity is the whole point of the shared GameController dispatch.
"""

import json
import os
import socket
import subprocess
import time

SOCKET_PATH = os.environ.get("CTP2_SMOKE_SOCKET", "/tmp/ctp2-smoke.sock")


class Ctp2Error(Exception):
    """Any harness-level failure (process died, timeout, bad response)."""


class Ctp2Client:
    def __init__(
        self,
        binary,
        mode,
        *,
        seed=42,
        players=4,
        socket_path=SOCKET_PATH,
        cwd=None,
        env=None,
        log_path=None,
        timeout=120,
        socket_wait=60,
    ):
        if mode == "headless":
            args = [binary, "--serve", "--players", str(players), "--seed", str(seed)]
        elif mode == "ui":
            args = [binary, "--smoke-test"]
        else:
            raise ValueError(f"unknown mode: {mode!r}")

        self.mode = mode
        self.socket_path = socket_path
        self.timeout = timeout
        # The binary resolves game data relative to its CWD. Binaries live in
        # <repo>/build/<exe>; data sits at <repo>. Default CWD to <repo>.
        self.cwd = cwd or os.path.dirname(os.path.dirname(os.path.abspath(binary)))

        # A stale socket from a crashed run would make connect() hit nothing.
        try:
            os.unlink(socket_path)
        except FileNotFoundError:
            pass

        self._log = open(log_path, "w") if log_path else subprocess.DEVNULL
        self.proc = subprocess.Popen(
            args, cwd=self.cwd, env=env, stdout=self._log, stderr=subprocess.STDOUT
        )

        try:
            self._wait_for_socket(socket_wait)
            self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.sock.connect(socket_path)
            self.sock.settimeout(timeout)
        except Exception:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
            if self._log not in (None, subprocess.DEVNULL):
                self._log.close()
            raise
        self._buf = b""

    # -- lifecycle --------------------------------------------------------

    def _wait_for_socket(self, timeout):
        deadline = time.time() + timeout
        while time.time() < deadline:
            if os.path.exists(self.socket_path):
                return
            if self.proc.poll() is not None:
                raise Ctp2Error(
                    f"process exited early (code {self.proc.returncode}) "
                    f"before opening the socket"
                )
            time.sleep(0.1)
        raise Ctp2Error("socket never appeared")

    def close(self):
        try:
            self._rpc("quit")
        except Exception:
            pass
        try:
            self.sock.close()
        except Exception:
            pass
        try:
            self.proc.wait(timeout=10)
        except Exception:
            self.proc.kill()
            self.proc.wait()
        if self._log not in (None, subprocess.DEVNULL):
            self._log.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    # -- protocol ---------------------------------------------------------

    def _rpc(self, line):
        self.sock.sendall((json.dumps({"cmd": line}) + "\n").encode())
        while b"\n" not in self._buf:
            chunk = self.sock.recv(65536)
            if not chunk:
                self.proc.poll()
                suffix = (
                    f" (process exit code {self.proc.returncode})"
                    if self.proc.returncode is not None
                    else ""
                )
                raise Ctp2Error(f"connection closed during '{line}'{suffix}")
            self._buf += chunk
        raw, self._buf = self._buf.split(b"\n", 1)
        return json.loads(raw)

    def command(self, verb, *args):
        """Send a verb (+ optional args) and return the parsed response dict."""
        line = verb if not args else verb + " " + " ".join(str(a) for a in args)
        return self._rpc(line)

    # query is just a command that reads; aliased for readable test code.
    query = command

    def expect_ok(self, verb, *args):
        r = self.command(verb, *args)
        if r.get("status") != "ok":
            raise Ctp2Error(f"{verb} {args} failed: {r}")
        return r

    def result(self, verb, *args):
        """expect_ok and return the 'result' object."""
        return self.expect_ok(verb, *args).get("result", {})

    # -- helpers ----------------------------------------------------------

    def wait_until(self, predicate, *, timeout=120, interval=0.25, desc=""):
        deadline = time.time() + timeout
        while time.time() < deadline:
            if self.proc.poll() is not None:
                raise Ctp2Error(f"process died while waiting for {desc}")
            if predicate():
                return
            time.sleep(interval)
        raise Ctp2Error(f"timeout after {timeout}s waiting for {desc}")

    def wait_game_loaded(self, *, timeout=120):
        """Block until a game is loaded. No-op-fast on headless (start_game is
        synchronous); covers the UI build's asynchronous menu->game load."""
        self.wait_until(
            lambda: self.query("query_cities").get("status") == "ok",
            timeout=timeout,
            desc="game loaded",
        )


def fixture_save(name):
    """Decompress test/fixtures/<name>.json.gz to /tmp and return the path.

    Campaign saves are kept gzipped in the repo (~90KB each instead of
    ~4.5MB); load_game reads plain JSON, so scenarios inflate on demand.
    """
    import gzip
    import shutil

    src = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "fixtures", name + ".json.gz")
    dst = f"/tmp/ctp2_fixture_{name}.json"
    with gzip.open(src, "rb") as fin, open(dst, "wb") as fout:
        shutil.copyfileobj(fin, fout)
    return dst
