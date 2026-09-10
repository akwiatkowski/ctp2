#!/usr/bin/env python3
"""
Scenario: sequential loads over a running game, with turns on each.

Promoted from the throwaway /tmp/load_stress.py that verified the
load-game crash fixes (serveRound clock stomp; std::string(nullptr) on
NULL Slic segment names during AutoSave after reload). Real campaign
saves exercise state new_game-based slices cannot reach: runtime Slic
symbols, mid-game AI, cities with production, armies with orders.

    load campaign-round160 -> 15 turns -> queries
    load rematch3-r89      -> 15 turns -> queries   (the r89 crash save)
    load campaign-round160 -> 10 turns -> queries

Usage: test/scenario_load_stress.py <path-to-ctp2_headless>
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error, fixture_save  # noqa: E402

PHASES = [
    ("campaign-round160", 15),
    ("rematch3-r89", 15),
    ("campaign-round160", 10),
]


def run(client):
    for name, turns in PHASES:
        path = fixture_save(name)
        client.expect_ok("load_game", path)
        round0 = client.result("query_turn")["round"]
        client.expect_ok("end_turn", turns)
        round1 = client.result("query_turn")["round"]
        assert round1 == round0 + turns, (
            f"{name}: clock broken, round {round0} + {turns} -> {round1}"
        )
        players = client.result("query_players")["players"]
        assert any(p.get("human") for p in players), f"{name}: human player lost"
        # query_player_cities walks city names through the UTF-8 path
        # (the Djenné regression lives here on the rematch save).
        client.expect_ok("query_player_cities", 2)
        print(f"  {name}: round {round0} -> {round1}, queries ok")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    ap.add_argument("--startup-timeout", type=int, default=60)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_scenario_load_stress.log"
    print(f"[load-stress] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(
            args.binary,
            "headless",
            log_path=log,
            socket_wait=args.startup_timeout,
        ) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[load-stress] FAIL: {e}")
        print(f"[load-stress] see game log: {log}")
        return 1
    print("[load-stress] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
