#!/usr/bin/env python3
"""
Auto-explore slice — HEADLESS ONLY.

auto_explore must keep a HUMAN army moving across turns: the per-turn tick
re-targets the nearest REACHABLE unexplored tile (passability-aware BFS),
walks the explored frontier when pathfinding refuses the unexplored target,
and steps straight into adjacent fog with a point MOVE_TO order. Regression
for the bug where human auto-explore cleared itself on the first tick
(pathfinding always refuses unexplored destinations for humans).

Usage:
    test/explore.py <path-to-ctp2_headless> [--log FILE]
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402


def run(client):
    client.expect_ok("new_game")
    client.expect_ok("start_game")
    client.wait_game_loaded()
    client.expect_ok("build_city")
    client.expect_ok("set_production", 0, "settler")
    for _ in range(10):
        client.expect_ok("end_turn", 5)
        armies = client.result("query_armies")["armies"]
        if any(a["can_settle"] for a in armies):
            break
    army = next(a for a in armies if a["can_settle"])

    explored0 = client.result("query_map")["explored"]
    client.expect_ok("auto_explore", army["index"])

    positions = []
    for _ in range(15):
        client.expect_ok("end_turn")
        armies = client.result("query_armies")["armies"]
        assert armies, "exploring settler vanished"
        a = armies[-1]
        positions.append((a["pos"]["x"], a["pos"]["y"]))
    explored1 = client.result("query_map")["explored"]

    distinct = len(set(positions))
    gained = explored1 - explored0
    print(f"  visited {distinct} distinct tiles, explored {explored0} -> {explored1} (+{gained})")
    assert distinct >= 4, f"settler barely moved: {positions}"
    assert gained >= 10, f"exploration ineffective: +{gained} tiles"

    # Manual orders override auto-explore: the next move_army must stick.
    a = client.result("query_armies")["armies"][-1]
    cur = (a["pos"]["x"], a["pos"]["y"])
    tiles = {(t["x"], t["y"]) for t in client.result("query_map")["tiles"]}
    step = next(c for c in tiles
                if max(abs(c[0] - cur[0]), abs(c[1] - cur[1])) == 1
                and client.command("move_army", a["index"], c[0], c[1]).get("status") == "ok")
    client.expect_ok("end_turn")
    after = client.result("query_armies")["armies"][-1]
    assert (after["pos"]["x"], after["pos"]["y"]) == step, (
        f"manual move overridden by auto-explore: wanted {step}, got {after['pos']}"
    )
    print(f"  manual override ok: settled course to {step}")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_explore_headless.log"
    print(f"[explore] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, "headless", log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[explore] FAIL: {e}")
        print(f"[explore] see game log: {log}")
        return 1
    print("[explore] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
