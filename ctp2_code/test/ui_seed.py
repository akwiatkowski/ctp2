#!/usr/bin/env python3
"""UI scene seeds must reproduce terrain and starting armies across processes."""
import json
import os
from pathlib import Path
import sys
from ctp2_client import Ctp2Client

binary = Path(sys.argv[1]).resolve()
out = binary.parent / "ui-seed"
out.mkdir(exist_ok=True)
states = []
for index, seed in enumerate((42, 42, 43)):
    socket = f"/tmp/ctp2-ui-seed-{os.getpid()}.sock"
    env = dict(os.environ, CTP2_SMOKE_SOCKET=socket)
    with Ctp2Client(str(binary), "ui", seed=seed, socket_path=socket, env=env,
                    log_path=str(out / f"{index}.log")) as client:
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded()
        world = client.result("query_world")
        state = {"terrain": world["terrain"], "width": world["width"], "height": world["height"],
                 "armies": client.result("query_armies")["armies"]}
        (out / f"{index}.json").write_text(json.dumps(state, indent=2))
        client.expect_ok("save_game", out / f"{index}.sav")
        states.append(state)
assert states[0] == states[1], f"same seed produced different scenes: {out}"
assert states[0]["terrain"] != states[2]["terrain"], "different seed was ignored"
print("PASS: UI seeds reproduce terrain and starting armies; a different seed changes the map")
