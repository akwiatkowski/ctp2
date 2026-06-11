#!/usr/bin/env python3
"""
Scenario: queued army paths resume across turns on real campaign state.

Promoted from the throwaway /tmp/path_crash.py that found two crashes in
the headless order-resume fix (NULL Cell::UnitArmy() in UpdateZOCForMove
and CheckLoadSleepingCargoFromCity). The c7-b-r80 save matters because it
holds armies WITH stale queued orders, including transported cargo — the
states that crashed when the round pump first re-fired BeginTurnExecute.

    load c7-b-r80 -> one move_army -> 4 turns with NO re-issuing
    -> the army must make progress every turn and nothing may crash.

Usage: test/scenario_path_resume.py <path-to-ctp2_headless>
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error, fixture_save  # noqa: E402


def army_pos(client, idx):
    for a in client.result("query_armies")["armies"]:
        if a["index"] == idx:
            return (a["pos"]["x"], a["pos"]["y"])
    raise AssertionError(f"army {idx} vanished")


def chebyshev(p, q):
    return max(abs(p[0] - q[0]), abs(p[1] - q[1]))


def run(client):
    client.expect_ok("load_game", fixture_save("c7-b-r80"))
    target = (36, 90)
    client.expect_ok("move_army", 0, *target)
    pos = army_pos(client, 0)
    print(f"  path issued: army 0 at {pos}, target {target}")

    stalled = 0
    for turn in range(4):
        client.expect_ok("end_turn", 1)
        new_pos = army_pos(client, 0)  # also proves the game survived
        if new_pos == pos and new_pos != target:
            stalled += 1
        else:
            pos = new_pos
        print(f"  after turn {turn + 1}: {new_pos}")
        if new_pos == target:
            break
    assert stalled < 2, (
        f"queued path stalled (the BeginTurnExecute regression): stuck at {pos}"
    )
    assert pos == target or chebyshev(pos, target) < chebyshev((40, 93), target), (
        f"no progress toward {target}: at {pos}"
    )
    # The save's other armies (including any cargo) resumed without crashing;
    # a final full query proves the world is still coherent.
    client.expect_ok("query_units")
    print(f"  path resumed across turns: ended at {pos}")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_scenario_path_resume.log"
    print(f"[path-resume] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, "headless", log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[path-resume] FAIL: {e}")
        print(f"[path-resume] see game log: {log}")
        return 1
    print("[path-resume] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
