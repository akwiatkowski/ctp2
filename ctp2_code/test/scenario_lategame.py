#!/usr/bin/env python3
"""
Scenario: forced late-game content — an undersea city, centuries early.

The nano-age is organically unreachable in test budgets (a 500-round soak
never gets close), so its content shipped untested: no test had ever
founded a water city. This scenario uses the two DEBUG cheats
(grant_advance + create_unit) to force the state directly:

    new game -> found the land capital (normal path)
    -> grant ADVANCE_NANO_ASSEMBLY (enables the Sea Engineer)
    -> create_unit UNIT_SEA_ENGINEER on open water
    -> build_city THERE -> a second city exists ON A WATER TILE.

That exercises the SettleWater path end-to-end: UDUnitTypeCanSettle's
water branch, city creation on water terrain, and the city queries over
the result — the exact mechanic mapped in the 2026-07-10 ocean-cities
investigation.

Usage: test/scenario_lategame.py <path-to-ctp2_headless>
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402

# Terrain DB indices (ctp2_data/default/gamedata/Terrain.txt, 0-based):
# 10 shallow water, 11 deep water, 14 shelf, 22 kelp, 23 reef.
WATER = {10, 11, 14, 22, 23}


def found_capital(client):
    def attempt():
        r = client.command("build_city")
        if r.get("status") == "ok":
            return True
        if r.get("detail") in ("game_not_loaded", "no_settler_found"):
            return False
        if "no_moves_left" in str(r.get("detail", "")):
            client.expect_ok("end_turn", 1)
            return False
        raise Ctp2Error(f"build_city failed hard: {r}")
    client.wait_until(attempt, timeout=60, desc="capital founded")


def pick_water_tile(client, min_city_dist=5):
    w = client.result("query_world")
    W, H, terrain = w["width"], w["height"], w["terrain"]
    cities = [(c["x"], c["y"]) for c in w.get("cities", [])]

    def far_from_cities(x, y):
        return all(max(abs(x - cx), abs(y - cy)) >= min_city_dist
                   for cx, cy in cities)

    for y in range(2, H - 2):
        for x in range(2, W - 2):
            if terrain[y * W + x] in WATER and far_from_cities(x, y):
                return x, y, terrain[y * W + x]
    raise Ctp2Error("no suitable water tile found on this map")


def run(client):
    client.expect_ok("new_game")
    client.expect_ok("start_game")
    client.wait_game_loaded()

    found_capital(client)
    n0 = len(client.result("query_cities")["cities"])
    assert n0 == 1, f"expected 1 city after founding, got {n0}"

    r = client.expect_ok("grant_advance", "ADVANCE_NANO_ASSEMBLY")
    print(f"  granted: {r['result']['name']} (id {r['result']['id']})")

    x, y, terr = pick_water_tile(client)
    print(f"  water tile: ({x},{y}) terrain {terr}")
    client.expect_ok("create_unit", "UNIT_SEA_ENGINEER", x, y)

    # build_city settles with the FIRST settler-capable army it finds; the
    # start-of-game units include another settler idling on the capital
    # tile (-> "tile_occupied"). Disband every army that is not our sea
    # engineer so the water settle is the only candidate.
    for _ in range(12):
        others = [a for a in client.result("query_armies")["armies"]
                  if (a["pos"]["x"], a["pos"]["y"]) != (x, y)]
        if not others:
            break
        client.expect_ok("disband_unit", others[0]["index"])

    # A freshly-created unit may need a turn boundary for special-action
    # points; build_city retries through that.
    def settle():
        rb = client.command("build_city")
        if rb.get("status") == "ok":
            pos = rb["result"]["pos"]
            assert (pos["x"], pos["y"]) == (x, y), (
                f"city founded at {pos}, expected water tile ({x},{y})"
            )
            return True
        if "no_moves_left" in str(rb.get("detail", "")) or \
           rb.get("detail") == "no_settler_found":
            client.expect_ok("end_turn", 1)
            return False
        raise Ctp2Error(f"undersea build_city failed hard: {rb}")
    client.wait_until(settle, timeout=60, desc="undersea city founded")

    cities = client.result("query_cities")["cities"]
    mine = [c for c in cities if (c["pos"]["x"], c["pos"]["y"]) == (x, y)]
    assert mine, f"no city at the water tile in query_cities: {cities}"
    # The tile must still be water in the world grid — an undersea city
    # must not silently terraform its tile to land.
    w = client.result("query_world")
    t_now = w["terrain"][y * w["width"] + x]
    assert t_now in WATER, (
        f"water tile ({x},{y}) changed terrain to {t_now} after founding"
    )
    print(f"  undersea city '{mine[0]['name']}' founded on water terrain "
          f"{t_now} — SettleWater path works")

    # And the game keeps running on top of it.
    client.expect_ok("end_turn", 3)
    assert any((c["pos"]["x"], c["pos"]["y"]) == (x, y)
               for c in client.result("query_cities")["cities"]), (
        "undersea city vanished within 3 rounds"
    )
    print("  survived 3 rounds")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_scenario_lategame.log"
    print(f"[lategame] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, "headless", players=2, seed=42,
                        log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[lategame] FAIL: {e}")
        print(f"[lategame] see game log: {log}")
        return 1
    print("[lategame] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
