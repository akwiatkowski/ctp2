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


def run_slice(client):
    client.expect_ok("new_game")
    client.expect_ok("start_game")
    client.wait_game_loaded()

    found_city(client)

    cities = client.result("query_cities")["cities"]
    assert len(cities) == 1, f"expected 1 city after founding, got {len(cities)}"
    name = cities[0]["name"]
    pos = cities[0]["pos"]
    print(f"  founded '{name}' at ({pos['x']},{pos['y']})")

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
            run_slice(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[slice] FAIL ({args.mode}): {e}")
        print(f"[slice] see game log: {log}")
        return 1

    print(f"[slice] PASS ({args.mode})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
