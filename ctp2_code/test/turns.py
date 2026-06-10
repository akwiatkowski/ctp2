#!/usr/bin/env python3
"""
Turn-advancement slice — HEADLESS ONLY.

Proves the keystone of time-dependent testing: end_turn advances full rounds
through the real event pipeline (AI players act), query_turn tracks the round,
and the world stays consistent and queryable afterwards.

UI build is excluded on purpose: its end_turn queues director->AddEndTurn for
the human player asynchronously (turn completion depends on the render loop),
so "advance N rounds synchronously" is a headless-only contract.

Usage:
    test/turns.py <path-to-ctp2_headless> [--log FILE]
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402

ROUNDS = 3


def run(client):
    client.expect_ok("new_game")
    client.expect_ok("start_game")
    client.wait_game_loaded()

    t0 = client.result("query_turn")
    assert t0["round"] == 0, f"fresh game should be at round 0, got {t0}"
    print(f"  start: round {t0['round']}, year {t0['year']}")

    # Found the human's city first so it can work/grow while time passes.
    client.expect_ok("build_city")

    r = client.command("end_turn", ROUNDS)
    assert r.get("status") == "ok", f"end_turn failed: {r}"
    print(f"  end_turn {ROUNDS}: {r.get('detail')}")

    t1 = client.result("query_turn")
    assert t1["round"] == ROUNDS, (
        f"query_turn should report round {ROUNDS} after end_turn {ROUNDS}, got {t1}"
    )
    print(f"  after: round {t1['round']}, year {t1['year']}")

    # Single-round form (no argument) advances exactly one more.
    client.expect_ok("end_turn")
    t2 = client.result("query_turn")
    assert t2["round"] == ROUNDS + 1, f"bare end_turn should advance 1, got {t2}"

    # The world survived: players still listed, the human city still exists,
    # and per-turn yields are reported (city worked during the rounds).
    players = client.result("query_players")["players"]
    assert any(p["human"] and not p["dead"] for p in players), "human player vanished"

    cities = client.result("query_cities")["cities"]
    assert len(cities) >= 1, "human city disappeared after turns"

    human = next(p for p in players if p["human"])
    detail = client.result("query_player", human["id"])
    assert detail["id"] == human["id"] and detail["num_cities"] >= 1, (
        f"query_player disagrees with query_players: {detail}"
    )

    mine = client.result("query_player_cities", human["id"])["cities"]
    assert mine and "yields" in mine[0], f"city yields missing: {mine}"
    y = mine[0]["yields"]
    print(
        f"  city yields after {t2['round']} rounds: food {y['food']}, "
        f"production {y['production']}, gold {y['gold']}, science {y['science']}"
    )

    # bad args stay errors
    assert client.command("end_turn", 0).get("detail") == "bad_args"
    assert client.command("end_turn", 9999).get("detail") == "bad_args"


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_turns_headless.log"
    print(f"[turns] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, "headless", log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[turns] FAIL: {e}")
        print(f"[turns] see game log: {log}")
        return 1
    print("[turns] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
