#!/usr/bin/env python3
"""Headless auto-explore stops on enemy sighting, but not on allied contact.

Usage: test/explore_contact.py <path-to-ctp2_headless> [--log FILE]
"""

import argparse
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402


def save_frontier(client, path):
    """Build the same active settler used by explore.py and sample its first reveal."""
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
    explorer = next(a for a in armies if a["can_settle"])
    original = explorer["pos"]
    initial_tiles = client.result("query_map")["tiles"]
    known = {(t["x"], t["y"]) for t in initial_tiles if t["visible"]}
    explored = {(t["x"], t["y"]) for t in initial_tiles}
    occupied = {(u["pos"]["x"], u["pos"]["y"])
                for u in client.result("query_units")["units"]}
    land = {t["id"] for t in client.result("query_terrains")["terrains"] if t["land"]}
    client.expect_ok("save_game", path)

    client.expect_ok("auto_explore", explorer["index"])
    assert next(a for a in client.result("query_armies")["armies"]
                if a["id"] == explorer["id"])["exploring"], "baseline explore did not start"
    # The nearest unexplored target itself is not a valid fixture position:
    # parking an allied unit on it can make the initial explore path impossible.
    ordered_path = path.replace(".json", ".ordered.json")
    client.expect_ok("save_game", ordered_path)
    with open(ordered_path) as saved:
        ordered = json.load(saved)
    unit_id = explorer["units"][0]["id"]
    target = next(u["explore_target"] for u in ordered["unit_pool"]["units"]
                  if u["id"] == unit_id)
    target_pos = (target["x"], target["y"])
    for turn in range(1, 5):
        client.expect_ok("end_turn")
        current = next(a for a in client.result("query_armies")["armies"]
                       if a["id"] == explorer["id"])
        if current["pos"] == original:
            continue
        # Place an AI unit where this explorer's movement actually exposes fog.
        # Choose a land tile away from its destination, not a tile already
        # occupied by any visible unit or city.
        candidates = [t for t in client.result("query_map")["tiles"]
                      if t["visible"] and (t["x"], t["y"]) not in known
                      and (t["x"], t["y"]) != target_pos
                      and (t["x"], t["y"]) not in occupied
                      and (t["x"], t["y"]) != (current["pos"]["x"], current["pos"]["y"])
                      and "city" not in t and t["terrain"] in land]
        if candidates:
            # Prefer fogged explored terrain (not a new explore target) near
            # the explorer so the planted army is sighted without blocking
            # its planned route.
            here = current["pos"]
            contact = min(
                candidates,
                key=lambda t: ((t["x"], t["y"]) not in explored,
                               abs(t["x"] - here["x"]) + abs(t["y"] - here["y"])),
            )
            return explorer["id"], (contact["x"], contact["y"]), turn
    raise AssertionError("baseline settler never exposed an unoccupied land tile")


def explore(client, path, explorer_id, contact_pos, enemy_owner, allied, deadline):
    client.expect_ok("load_game", path)
    if allied:
        client.expect_ok("debug_form_alliance", enemy_owner)
    contact_id = client.result("debug_spawn_contact", enemy_owner, *contact_pos)["unit_id"]

    armies = client.result("query_armies")["armies"]
    army = next(a for a in armies if a["id"] == explorer_id)
    idx = army["index"]
    assert not army["exploring"]
    assert contact_id not in {u["id"] for u in client.result("query_units")["units"]}, (
        "enemy already visible before exploring"
    )

    client.expect_ok("auto_explore", idx)
    assert next(a for a in client.result("query_armies")["armies"]
                if a["id"] == explorer_id)["exploring"], (
        f"auto_explore did not seed a {'allied' if allied else 'nonallied'} course "
        f"with contact at {contact_pos}"
    )
    moved = False
    start = army["pos"]
    trail = []
    for turn in range(deadline + 2):
        armies = client.result("query_armies")["armies"]
        army = next(a for a in armies if a["id"] == explorer_id)
        moved |= army["pos"] != start
        trail.append((army["pos"], army["exploring"]))
        if contact_id in {u["id"] for u in client.result("query_units")["units"]}:
            assert moved, "contact revealed without the explorer moving"
            assert army["exploring"] == allied, (
                f"auto-explore {'stopped for ally' if allied else 'continued after enemy sighting'} "
                f"on turn {turn} at {army['pos']}"
            )
            if not allied:
                stopped = army["pos"]
                for _ in range(2):
                    client.expect_ok("end_turn")
                    army = next(a for a in client.result("query_armies")["armies"]
                                if a["id"] == explorer_id)
                    assert not army["exploring"] and army["pos"] == stopped, (
                        "explorer resumed its old move order after enemy contact"
                    )
            print(f"  {'allied' if allied else 'enemy'} contact at {army['pos']}: "
                  f"exploring={army['exploring']}")
            return
        # The full turn also runs AI diplomacy. The allied case steps the real
        # army order event directly to isolate sighting from unrelated AI.
        if allied:
            client.expect_ok("debug_step_explore", idx)
        else:
            client.expect_ok("end_turn")
    raise AssertionError(f"explorer did not reveal contact within {deadline+2} turns: {trail}")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default="/tmp/ctp2_explore_contact_headless.log")
    args = ap.parse_args()
    try:
        with tempfile.TemporaryDirectory(prefix="ctp2-explore-contact-") as folder:
            path = os.path.join(folder, "baseline.json")
            with Ctp2Client(args.binary, "headless", log_path=args.log) as client:
                explorer_id, pos, deadline = save_frontier(client, path)
                owner = next(p["id"] for p in client.result("query_players")["players"]
                             if p["id"] > 0 and not p["human"] and not p["dead"])
            for allied in (False, True):
                with Ctp2Client(args.binary, "headless", log_path=args.log) as client:
                    explore(client, path, explorer_id, pos, owner, allied, deadline)
    except (Ctp2Error, AssertionError) as error:
        print(f"[explore-contact] FAIL: {error}\nsee game log: {args.log}")
        return 1
    print("[explore-contact] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
