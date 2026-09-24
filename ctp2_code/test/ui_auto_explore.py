#!/usr/bin/env python3
"""The real Move hotkey cancels an army's pending auto-explore path."""
from pathlib import Path
import sys

import ui_scenario

scenario = ui_scenario.launch(Path(sys.argv[1]), "ui-auto-explore", scrub_gpu=True)
print(f"Auto-explore UI artifacts: {scenario.out}", flush=True)

with scenario.connect(timeout=15) as ui:
    client = ui.client
    ui.start_new_game(seed=42, players=4)
    army = next(a for a in client.result("query_armies")["armies"] if a["moves_left"] > 0)
    client.expect_ok("debug_actor_state", army["index"], 0, "select")
    client.expect_ok("auto_explore", army["index"])

    def current():
        return next(a for a in client.result("query_armies")["armies"]
                    if a["id"] == army["id"])

    assert current()["exploring"], "auto-explore never started"
    client.result("screenshot_frame", scenario.out / "exploring.bmp")
    # Auto-explore may auto-select another idle army. Reselect the exploring
    # unit before pressing Move, as a player would.
    client.expect_ok("debug_actor_state", army["index"], 0, "select")
    # Seed 42's first settler is at this screen position in testprofile's
    # pinned 1024x768 layout. Inspect its real right-click menu while active.
    for down in (0, 1, 0):
        client.expect_ok("ui_pointer", 630, 430, 0, down)
    client.result("screenshot_frame", scenario.out / "active-menu.bmp")
    ui.click(400, 120)  # Dismiss the popup before exercising the Move key.
    client.expect_ok("debug_actor_state", army["index"], 0, "select")
    # default/uidata/keymap.txt binds lowercase m to MOVE_ORDER.
    client.expect_ok("ui_key", "m")
    assert not current()["exploring"], "Move key left auto-explore active"
    client.result("screenshot_frame", scenario.out / "move-key.bmp")
print("PASS Move key cancels auto-explore and keeps UI responsive", flush=True)
