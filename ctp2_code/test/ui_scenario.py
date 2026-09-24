#!/usr/bin/env python3
"""Shared launcher for the windowless UI integration scenarios.

Each ui_*.py script still owns its assertions, timeouts and artifact layout;
this module only owns the duplicated plumbing: the artifact directory beside
the binary, a unique smoke socket, the SDL dummy-driver environment, optional
testprofile.txt overrides, the Ctp2Client lifecycle, and the real-pointer
click/New Game helpers every scenario builds on.

Usage:

    scenario = ui_scenario.launch(binary, "ui-edge-scroll")
    with scenario.connect() as ui:
        ui.start_new_game(seed=42, players=4)
        ui.click_control("ControlPanelWindow.ControlPanel.TurnButton")
        client = ui.client  # the raw Ctp2Client for everything else
"""

import os
from pathlib import Path
import tempfile
import uuid

from ctp2_client import Ctp2Client


NEW_GAME = "InitPlayWindow.NewGameButton"
START = "SPNewGameWindow.StartButton"
RADAR = "RadarWindow.RadarMap"

TEST_PROFILE = Path(__file__).with_name("testprofile.txt")


def write_profile(out, settings):
    """Copy testprofile.txt into `out` with `settings` keys replaced/appended."""
    lines = TEST_PROFILE.read_text().splitlines()
    lines = [line for line in lines if line.split("=", 1)[0] not in settings]
    profile = out / "profile.txt"
    profile.write_text("\n".join(lines + [f"{k}={v}" for k, v in settings.items()]) + "\n")
    return profile


def build_env(socket_path, *, profile=None, extra_env=None, scrub_gpu=False):
    """The standard windowless environment for a UI scenario child process."""
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_VIDEODRIVER="dummy",
               SDL_AUDIO_DRIVER="dummy", SDL_AUDIODRIVER="dummy",
               SDL_RENDER_DRIVER="software", CTP2_CAPTURE_FRAMES="1",
               CTP2_PROFILE=str(profile or TEST_PROFILE),
               CTP2_SMOKE_SOCKET=socket_path)
    if scrub_gpu:
        # An inherited developer opt-out must not silently select a renderer.
        for key in env:
            if key.startswith("CTP2_GPU_") or key == "CTP2_MODERN_SPRITES":
                env[key] = ""
    env.update(extra_env or {})
    return env


def launch(binary, name, *, settings=None, extra_env=None, scrub_gpu=False):
    """Prepare a scenario launch: artifact dir `name`-* beside the binary, a
    unique socket, and the dummy-driver env. `settings` are merged over
    testprofile.txt into <out>/profile.txt; `scrub_gpu` blanks inherited
    CTP2_GPU_*/CTP2_MODERN_SPRITES; `extra_env` is applied last."""
    return UiScenario(binary, name, settings=settings,
                      extra_env=extra_env, scrub_gpu=scrub_gpu)


class UiScenario:
    """A prepared launch. connect() spawns the game and returns a UiSession;
    call it again to restart the binary with identical settings."""

    def __init__(self, binary, name, *, settings=None, extra_env=None, scrub_gpu=False):
        self.binary = Path(binary).resolve()
        self.out = Path(tempfile.mkdtemp(prefix=f"{name}-", dir=self.binary.parent))
        self.socket = f"/tmp/ctp2-{name}-{uuid.uuid4().hex[:16]}.sock"
        profile = write_profile(self.out, settings) if settings else None
        self.env = build_env(self.socket, profile=profile,
                             extra_env=extra_env, scrub_gpu=scrub_gpu)

    def connect(self, *, seed=42, players=4, timeout=120, log_name="game.log"):
        client = Ctp2Client(str(self.binary), "ui", seed=seed, players=players,
                            env=self.env, socket_path=self.socket,
                            log_path=str(self.out / log_name), timeout=timeout)
        return UiSession(self, client)


class UiSession:
    """A live client plus the shared control/click/new-game helpers.

    Assign `record` a callable(event, **fields) to tee helper activity into
    the scenario's own trace (see ui_offscreen/ui_sprite_clicks).
    """

    def __init__(self, scenario, client):
        self.scenario = scenario
        self.client = client
        self.out = scenario.out
        self.record = None

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return self.client.__exit__(*exc)

    def control(self, path, *, timeout=30, interval=0.05):
        """Wait for a visible, enabled, non-empty control; return its bounds."""
        bounds = {}

        def ready():
            nonlocal bounds
            response = self.client.command("ui_control_bounds", path)
            bounds = response.get("result", {})
            return (response.get("status") == "ok"
                    and bounds.get("visible") and bounds.get("enabled")
                    and bounds.get("width", 0) > 0
                    and bounds.get("height", 0) > 0)

        self.client.wait_until(ready, timeout=timeout, interval=interval,
                               desc=f"visible enabled {path}")
        if self.record:
            self.record("control", path=path, bounds=bounds)
        return bounds

    def click(self, x, y):
        """One real pointer click (move, down, up) at pixel coordinates."""
        if self.record:
            self.record("click", x=x, y=y)
        for buttons in (0, 1, 0):
            self.client.expect_ok("ui_pointer", x, y, buttons)

    def click_control(self, path, *, timeout=30):
        """Wait for the control, then click its centre."""
        bounds = self.control(path, timeout=timeout)
        self.click(bounds["x"] + bounds["width"] // 2,
                   bounds["y"] + bounds["height"] // 2)

    def start_new_game(self, seed=42, players=4, *, load_timeout=120):
        """Drive the real menus: New Game -> prepare -> Start -> loaded."""
        self.click_control(NEW_GAME)
        self.control(START)
        self.client.expect_ok("ui_prepare_game", seed, players)
        self.click_control(START)
        self.client.wait_game_loaded(timeout=load_timeout)
