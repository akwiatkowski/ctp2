#!/usr/bin/env python3
"""
World-observability test — runs IDENTICALLY on ctp2 (UI) and ctp2_headless.

Asserts that the player can OBSERVE the world through fog of war: the map query
reports explored/visible tiles and city markers, the units query reports the
visible units, and these agree with each other and with query_cities.

It deliberately asserts only on stable facts (a founded city is visible to its
owner; explored/visible tile counts are positive; cross-query agreement) and
avoids turn-dependent or lazy-cleanup details (e.g. exactly when the settler is
removed), so it is robust on both builds without advancing any turns.

Usage:
    test/observability.py <path-to-binary> --mode {headless|ui} [--log FILE]
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402


def found_city(client):
    state = {}

    def attempt():
        r = client.command("build_city")
        state["last"] = r
        if r.get("status") == "ok":
            return True
        if r.get("detail") in ("game_not_loaded", "no_settler_found"):
            return False
        raise Ctp2Error(f"build_city failed hard: {r}")

    client.wait_until(attempt, timeout=60, desc="city founded")


def run(client):
    client.expect_ok("new_game")
    client.expect_ok("start_game")
    client.wait_game_loaded()

    # Before founding: the starting settler already reveals a patch of map.
    m0 = client.result("query_map")
    assert m0["width"] > 0 and m0["height"] > 0, f"bad map dims: {m0['width']}x{m0['height']}"
    assert m0["explored"] > 0, "player explores nothing at game start"
    assert m0["visible"] > 0, "player sees nothing at game start"
    assert len(m0["tiles"]) == m0["explored"], "tiles list != explored count"
    print(f"  start: {m0['width']}x{m0['height']} map, "
          f"{m0['explored']} explored / {m0['visible']} visible")

    u0 = client.result("query_units")
    mine = [u for u in u0["units"] if u["owner"] == u0["visible_player"]]
    assert mine, "player sees none of its own units at start"
    print(f"  start units (mine): {[(u['name'], u['type']) for u in mine]}")

    found_city(client)

    # The founded city must show up as a city marker on the player's own map,
    # owned by the player, on a currently-visible tile.
    m1 = client.result("query_map")
    city_tiles = [t for t in m1["tiles"] if "city" in t]
    assert len(city_tiles) >= 1, "no city marker on the map after founding"
    mine_cities = [t for t in city_tiles if t["city"] == m1["visible_player"]]
    assert mine_cities, f"founded city not owned by player on map: {city_tiles}"
    ct = mine_cities[0]
    assert ct["visible"], f"player can't see its own city tile: {ct}"
    assert m1["explored"] >= m0["explored"], "explored area shrank after founding"
    print(f"  after founding: city marker at ({ct['x']},{ct['y']}), "
          f"explored {m1['explored']} / visible {m1['visible']}")

    # Cross-query agreement: the map's city tile matches query_cities' position.
    cities = client.result("query_cities")["cities"]
    assert len(cities) == 1, f"expected 1 city, got {len(cities)}"
    cp = cities[0]["pos"]
    assert (cp["x"], cp["y"]) == (ct["x"], ct["y"]), (
        f"query_cities pos {cp} != query_map city tile ({ct['x']},{ct['y']})"
    )
    print(f"  cross-check: query_cities and query_map agree on ({cp['x']},{cp['y']})")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--mode", required=True, choices=["headless", "ui"])
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or f"/tmp/ctp2_observability_{args.mode}.log"
    print(f"[observability] {args.mode}: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, args.mode, log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[observability] FAIL ({args.mode}): {e}")
        print(f"[observability] see game log: {log}")
        return 1
    print(f"[observability] PASS ({args.mode})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
