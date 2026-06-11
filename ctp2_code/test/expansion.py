#!/usr/bin/env python3
"""
Expansion slice — HEADLESS ONLY. The full "second city" loop a player (or an
LLM driver) needs:

    build_city → set_production settler → end_turn until built →
    query_armies → move_army away → end_turn while walking →
    build_city → TWO cities.

Also regression-tests the settle guard: founding on a tile that already has
a city must FAIL with tile_occupied (it used to silently REPLACE the city —
found by the first MCP playtest).

Usage:
    test/expansion.py <path-to-ctp2_headless> [--log FILE]
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402


def settler_army(client):
    """The army index that can settle, or None."""
    for a in client.result("query_armies")["armies"]:
        if a["can_settle"]:
            return a
    return None


def run(client):
    client.expect_ok("new_game")
    client.expect_ok("start_game")
    client.wait_game_loaded()

    client.expect_ok("build_city")
    home = client.result("query_cities")["cities"][0]
    hx, hy = home["pos"]["x"], home["pos"]["y"]
    print(f"  founded {home['name']} at ({hx},{hy})")

    # Settle guard: the starting tile now has a city; a settler created ON it
    # must not be able to found another. Produce one and try.
    client.expect_ok("set_production", 0, "settler")
    for _ in range(10):
        client.expect_ok("end_turn", 5)
        if settler_army(client):
            break
    army = settler_army(client)
    assert army, "settler never appeared after 50 rounds"
    assert army["pos"] == {"x": hx, "y": hy}, f"settler not at home: {army}"

    r = client.command("build_city")
    assert r.get("detail") == "tile_occupied", (
        f"settling on own city tile must fail with tile_occupied, got: {r}"
    )
    cities = client.result("query_cities")["cities"]
    assert len(cities) == 1 and cities[0]["population"] >= 1, (
        f"home city damaged by refused settle: {cities}"
    )
    print(f"  settle guard ok: tile_occupied, {cities[0]['name']} intact "
          f"(pop {cities[0]['population']})")

    # March the settler away, explore-as-you-go: pathfinding (correctly)
    # refuses unexplored tiles and a settler walks 1 tile/turn, so each step
    # targets an EXPLORED tile adjacent to the current position that takes us
    # further from home; vision expands as we move. Stop at distance >= 2
    # (city minimum-distance rules veto adjacent founding).
    def dist(p, q):  # Chebyshev — CTP2 grids move in 8 directions
        return max(abs(p[0] - q[0]), abs(p[1] - q[1]))

    for _step in range(14):
        a = settler_army(client)
        assert a, "settler vanished while marching"
        cur = (a["pos"]["x"], a["pos"]["y"])
        if dist(cur, (hx, hy)) >= 3:
            break
        explored = {(t["x"], t["y"]) for t in client.result("query_map")["tiles"]}
        steps = sorted(
            (c for c in explored
             if dist(c, cur) == 1 and dist(c, (hx, hy)) > dist(cur, (hx, hy))),
            key=lambda c: -dist(c, (hx, hy)))
        moved = False
        for cand in steps:
            r = client.command("move_army", a["index"], cand[0], cand[1])
            if r.get("status") == "ok":
                moved = True
                break
        assert moved, f"no walkable step from {cur} (explored: {sorted(explored)})"
        client.expect_ok("end_turn")  # refresh movement points + vision
    a = settler_army(client)
    cur = (a["pos"]["x"], a["pos"]["y"])
    assert dist(cur, (hx, hy)) >= 3, f"settler stuck near home: {a}"
    print(f"  settler marched to {cur} (distance {dist(cur, (hx, hy))})")

    client.expect_ok("build_city")
    cities = client.result("query_cities")["cities"]
    assert len(cities) == 2, f"expected 2 cities after expansion, got {cities}"
    names = [c["name"] for c in cities]
    print(f"  EXPANSION COMPLETE: {names}")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_expansion_headless.log"
    print(f"[expansion] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, "headless", log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[expansion] FAIL: {e}")
        print(f"[expansion] see game log: {log}")
        return 1
    print("[expansion] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
