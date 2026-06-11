#!/usr/bin/env python3
"""
Save/load vertical-slice test — runs IDENTICALLY on ctp2 (UI) and ctp2_headless.

It drives the loop a player actually performs and asserts on what the player can
see and do, end to end:

    new game -> found a city -> save -> load -> the player still SEES the city
             -> read what it's building -> MODIFY current production.

Every recent fix on this branch was a save/load / render-after-load bug; this
test exercises exactly that path, so the same class of regression fails here
loudly instead of being eyeballed.

Absorbed the former observability.py slice (same boot on both binaries, two
fewer game launches — one of them the slow UI menu boot): the player can
OBSERVE the world through fog of war. query_map reports explored/visible
tiles and city markers, query_units reports the player's own units, and the
queries agree with each other and with query_cities. Asserts only on stable
facts (a founded city is visible to its owner; counts positive; cross-query
agreement), nothing turn-dependent.

Usage:
    test/slice_save_load.py <path-to-binary> --mode {headless|ui} [--log FILE]

Exit code 0 = pass, 1 = fail.
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402

SAVE_PATH = "/tmp/ctp2_slice.sav"


def found_city(client):
    """build_city after the game is ready. The starting settler may not be
    placed the instant the game loads (UI build), and build_city is a no-op
    error until it exists, so retry until it settles."""
    state = {}

    def attempt():
        r = client.command("build_city")
        state["last"] = r
        if r.get("status") == "ok":
            return True
        # Only these are "not ready yet"; anything else is a real failure.
        if r.get("detail") in ("game_not_loaded", "no_settler_found"):
            return False
        raise Ctp2Error(f"build_city failed hard: {r}")

    client.wait_until(attempt, timeout=60, desc="city founded")


def run_slice(client, mode):
    client.expect_ok("new_game")
    client.expect_ok("start_game")
    client.wait_game_loaded()

    # --- observability before founding (absorbed observability.py) ---
    # The starting settler already reveals a patch of map, and the player
    # sees its own units.
    m0 = client.result("query_map")
    assert m0["width"] > 0 and m0["height"] > 0, f"bad map dims: {m0['width']}x{m0['height']}"
    assert m0["explored"] > 0, "player explores nothing at game start"
    assert m0["visible"] > 0, "player sees nothing at game start"
    assert len(m0["tiles"]) == m0["explored"], "tiles list != explored count"
    u0 = client.result("query_units")
    my_units = [u for u in u0["units"] if u["owner"] == u0["visible_player"]]
    assert my_units, "player sees none of its own units at start"
    print(f"  start: {m0['width']}x{m0['height']} map, "
          f"{m0['explored']} explored / {m0['visible']} visible, "
          f"units (mine): {[(u['name'], u['type']) for u in my_units]}")

    found_city(client)

    cities = client.result("query_cities")["cities"]
    assert len(cities) == 1, f"expected 1 city after founding, got {len(cities)}"
    name = cities[0]["name"]
    pos = cities[0]["pos"]
    print(f"  founded '{name}' at ({pos['x']},{pos['y']})")

    # --- observability after founding ---
    # The founded city shows up as a city marker on the player's own map,
    # owned by the player, on a currently-visible tile — and the map agrees
    # with query_cities about where it is.
    m1 = client.result("query_map")
    city_tiles = [t for t in m1["tiles"] if "city" in t]
    assert city_tiles, "no city marker on the map after founding"
    mine_cities = [t for t in city_tiles if t["city"] == m1["visible_player"]]
    assert mine_cities, f"founded city not owned by player on map: {city_tiles}"
    ct = mine_cities[0]
    assert ct["visible"], f"player can't see its own city tile: {ct}"
    assert m1["explored"] >= m0["explored"], "explored area shrank after founding"
    assert (pos["x"], pos["y"]) == (ct["x"], ct["y"]), (
        f"query_cities pos {pos} != query_map city tile ({ct['x']},{ct['y']})"
    )
    print(f"  observability ok: city marker at ({ct['x']},{ct['y']}), "
          f"explored {m1['explored']} / visible {m1['visible']}")

    city = client.result("query_city", 0)
    buildable = city["buildable"]
    assert buildable, "city reports no buildable units"
    print(f"  building before save = {city['building']}; {len(buildable)} buildable")

    # Save, then load it straight back.
    client.expect_ok("save_game", SAVE_PATH)
    client.expect_ok("load_game", SAVE_PATH)

    # The regression guard: the city the player founded must survive the round
    # trip and still be visible.
    cities = client.result("query_cities")["cities"]
    assert len(cities) == 1, f"city vanished after load: {len(cities)} visible"
    assert cities[0]["name"] == name, f"city changed after load: {cities[0]['name']}"
    print(f"  after load: still see '{cities[0]['name']}'")

    # Modify current production — pick a buildable unit and confirm it took.
    target = buildable[-1]["type"]
    client.expect_ok("set_production", 0, target)
    after = client.result("query_city", 0)["building"]
    assert after and after["type"] == target, (
        f"production not applied: building={after}, wanted type {target}"
    )
    print(f"  set production -> unit type {target}; city now building {after}")

    # set_production REPLACES the queue (campaign 7: a granary queued
    # behind a 740-shield settler for 20 rounds). Setting a different
    # target must take effect immediately, not append behind the first.
    other = buildable[0]["type"]
    if other != target:
        client.expect_ok("set_production", 0, other)
        after = client.result("query_city", 0)["building"]
        assert after and after["type"] == other, (
            f"set_production appended instead of replacing: building={after}, "
            f"wanted type {other}"
        )
        print(f"  replace semantics ok: building switched to type {other}")

    # The remaining checks drive end_turn, which the UI binary's legacy
    # smoke verb does not take arguments for — and the clock bugs they
    # guard live in headless serve mode (serveRound was headless-only).
    if mode != "headless":
        return

    # Clock regression (the serveRound bug): end_turn AFTER load must
    # CONTINUE the loaded clock, not restart or stomp it backwards.
    round0 = client.result("query_turn")["round"]
    client.expect_ok("end_turn", 3)
    round1 = client.result("query_turn")["round"]
    assert round1 == round0 + 3, (
        f"clock broken after load: round {round0} + 3 turns -> {round1}"
    )
    print(f"  clock after load ok: round {round0} -> {round1}")

    # Sequential reload over a RUNNING game (the AutoSave/Slic-name crash
    # path): save the advanced state, reload the ORIGINAL save mid-game,
    # run turns, then reload the newer save and run again. Each load must
    # restore its own clock and survive the turns.
    save2 = SAVE_PATH + ".later"
    client.expect_ok("save_game", save2)
    client.expect_ok("load_game", SAVE_PATH)
    r = client.result("query_turn")["round"]
    assert r == round0, f"first save's clock not restored: {r} != {round0}"
    client.expect_ok("end_turn", 2)
    client.expect_ok("load_game", save2)
    r = client.result("query_turn")["round"]
    assert r == round1, f"second save's clock not restored: {r} != {round1}"
    client.expect_ok("end_turn", 2)
    assert client.result("query_turn")["round"] == round1 + 2
    cities = client.result("query_cities")["cities"]
    assert cities and cities[0]["name"] == name, "city lost across reloads"
    print(f"  sequential reloads ok: {round0} <-> {round1}, turns ran on both")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary", help="path to ctp2 or ctp2_headless")
    ap.add_argument("--mode", required=True, choices=["headless", "ui"])
    ap.add_argument("--log", default=None, help="capture game stdout/stderr here")
    args = ap.parse_args()

    log = args.log or f"/tmp/ctp2_slice_{args.mode}.log"
    print(f"[slice] {args.mode}: {args.binary} (game log -> {log})")

    try:
        with Ctp2Client(args.binary, args.mode, log_path=log) as client:
            run_slice(client, args.mode)
    except (Ctp2Error, AssertionError) as e:
        print(f"[slice] FAIL ({args.mode}): {e}")
        print(f"[slice] see game log: {log}")
        return 1

    print(f"[slice] PASS ({args.mode})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
