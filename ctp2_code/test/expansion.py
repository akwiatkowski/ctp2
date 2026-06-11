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

    # Too-close guard: a vetoed settle must NOT consume the settler. The
    # engine's settle event queues GEV_KillUnit before GEV_CreateCity, so
    # without the pre-check a rejected site destroys the unit (campaign 7
    # lost a 740-shield settler this way). Step one tile off the city and
    # try to settle iso-adjacent to it.
    moved = False
    for dx, dy in ((1, 0), (0, 1), (-1, 0), (0, -1), (1, 1), (-1, -1)):
        r = client.command("move_army", army["index"], hx + dx, hy + dy)
        if r.get("status") == "ok":
            moved = True
            break
    assert moved, "settler could not step off the city tile"
    client.expect_ok("end_turn", 1)
    a = settler_army(client)
    if a and a["pos"] != {"x": hx, "y": hy}:  # actually stepped off
        r = client.command("build_city")
        assert r.get("status") == "error" and "too_close" in r.get("detail", ""), (
            f"settling next to a city must fail with too_close_to_city: {r}"
        )
        assert settler_army(client), "rejected settle CONSUMED the settler"
        print("  too-close guard ok: settler survived the refusal")

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

    # Multi-turn path persistence: a queued path must survive end_turn
    # WITHOUT re-issuing the move. The interactive UI re-fires
    # GEV_BeginTurnExecute per army each turn; headless must do the same
    # (campaign 7: every queued path died after its first leg).
    explored = {(t["x"], t["y"]) for t in client.result("query_map")["tiles"]}
    # Stay >= 3 from home so the follow-up settle isn't blocked by the
    # too-close rule after this detour.
    # Prefer a target several tiles out so the route genuinely spans turns
    # (the first leg executes on issue; one-leg routes test nothing). Try
    # candidates farthest-first until pathfinding accepts one.
    candidates = sorted((c for c in explored
                         if dist(c, (hx, hy)) >= 3 and dist(c, cur) >= 2),
                        key=lambda c: -dist(c, cur))
    far = None
    for cand in candidates[:8]:
        if client.command("move_army", a["index"], cand[0], cand[1]).get("status") == "ok":
            far = cand
            break
    if far is not None:
        if True:  # route accepted above
            # move_army executes the first leg immediately; resumption is
            # about progress AFTER that, across end_turns we don't drive.
            a = settler_army(client)
            start = (a["pos"]["x"], a["pos"]["y"])
            cur = start
            for _ in range(8):
                client.expect_ok("end_turn")
                a = settler_army(client)
                assert a, "settler vanished on queued path"
                cur = (a["pos"]["x"], a["pos"]["y"])
                if cur == far:
                    break
            assert cur == far or dist(cur, far) < dist(start, far), (
                f"queued path did not resume after end_turn: at {cur}, "
                f"was {start}, target {far}"
            )
            print(f"  queued path persisted: {start} -> {cur} (target {far})")

    client.expect_ok("build_city")
    cities = client.result("query_cities")["cities"]
    assert len(cities) == 2, f"expected 2 cities after expansion, got {cities}"
    names = [c["name"] for c in cities]
    print(f"  EXPANSION COMPLETE: {names}")

    # Army-order honesty (campaign 7 regressions): produce one military army
    # and check the verbs that used to lie or were missing.
    client.expect_ok("set_production", 0, "cheapest_military")
    army = None
    for _ in range(12):
        client.expect_ok("end_turn", 3)
        armies = [a for a in client.result("query_armies")["armies"]
                  if not a["can_settle"]]
        if armies:
            army = armies[0]
            break
    assert army, "military army never appeared"
    idx = army["index"]

    # board with no transport anywhere must FAIL (it used to return blind ok
    # while the unit stayed ashore) and must not consume the army.
    r = client.command("board", idx)
    assert r.get("status") == "error" and r.get("detail") == "no_transport_in_range", (
        f"board without a transport must fail honestly: {r}"
    )
    assert any(a["index"] == idx for a in client.result("query_armies")["armies"]), (
        "army vanished after refused board"
    )
    print("  board honesty ok: no_transport_in_range, army intact")

    # fortify reports verified entrenchment state.
    r = client.command("fortify", idx)
    assert r.get("status") == "ok", f"fortify failed: {r}"
    st = r["result"]
    assert st.get("entrenched") or st.get("entrenching"), (
        f"fortify ok but no entrenchment state: {st}"
    )
    print(f"  fortify ok: {st}")


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
