#!/usr/bin/env python3
"""
Scenario: conquest ending — the game's END has never been tested.

Every other test starts games, runs turns, saves and loads — none has ever
finished one. This scenario forces a 2-player conquest with the DEBUG
cheats: wait for the lone AI opponent to found its city, drop a fusion-tank
strike force next to it, capture it, hunt down whatever units the AI has
left, and then assert the ENGINE RECOGNIZES THE ENDING — the opponent's
dead flag and/or the human's has_won_the_game — and keeps answering
queries and running rounds afterward (the classic endgame crash window).

Usage: test/scenario_victory.py <path-to-ctp2_headless>
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402

TANK = "UNIT_FUSION_TANK"
BARBARIAN = 0


def cheb(p, q):
    return max(abs(p[0] - q[0]), abs(p[1] - q[1]))


def me(client):
    return client.result("query_map")["visible_player"]


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


def enemy_state(client, my_id):
    """(enemy_id, enemy_cities, enemy_dead) from the fog-free world view +
    player flags."""
    w = client.result("query_world")
    cities = [c for c in w.get("cities", [])
              if c["owner"] not in (my_id, BARBARIAN)]
    players = client.result("query_players")["players"]
    enemies = [p for p in players
               if p["id"] not in (my_id, BARBARIAN)]
    assert enemies, "no opponent exists"
    e = enemies[0]
    return e["id"], cities, bool(e.get("dead")), bool(
        next(p for p in players if p["id"] == my_id).get("has_won_the_game"))


def spawn_strike_force(client, pos, n=4):
    """n fusion tanks on one free tile next to pos, grouped."""
    # Probe the 8 neighbors until create_unit accepts one.
    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1),
                   (1, 1), (-1, -1), (1, -1), (-1, 1)):
        x, y = pos[0] + dx, pos[1] + dy
        r = client.command("create_unit", TANK, x, y)
        if r.get("status") == "ok":
            for _ in range(n - 1):
                client.expect_ok("create_unit", TANK, x, y)
            return x, y
    raise Ctp2Error(f"no spawnable tile next to {pos}")


def army_at(client, pos):
    for a in client.result("query_armies")["armies"]:
        if (a["pos"]["x"], a["pos"]["y"]) == pos:
            return a
    return None


def run(client):
    client.expect_ok("new_game")
    client.expect_ok("start_game")
    client.wait_game_loaded()
    found_capital(client)
    my_id = me(client)

    # Wait for the AI to found its city (fog-free via query_world).
    for _ in range(60):
        enemy_id, ecities, edead, won = enemy_state(client, my_id)
        if ecities:
            break
        client.expect_ok("end_turn", 1)
    assert ecities, "AI never founded a city in 60 rounds"
    target = (ecities[0]["x"], ecities[0]["y"])
    print(f"  enemy {enemy_id} city at {target}")

    spawn = spawn_strike_force(client, target)
    client.expect_ok("declare_war", enemy_id)

    # Assault until the enemy holds no cities (recapture-proof loop).
    for rnd in range(30):
        enemy_id, ecities, edead, won = enemy_state(client, my_id)
        if not ecities:
            break
        target = (ecities[0]["x"], ecities[0]["y"])
        army = army_at(client, spawn)
        if army is None:
            spawn = spawn_strike_force(client, target)
            army = army_at(client, spawn)
        if cheb(spawn, target) == 1:
            r = client.command("attack", army["index"], *target)
            if r.get("status") != "ok":
                client.expect_ok("end_turn", 1)   # regain movement points
        else:
            # City moved out of reach (rare): respawn adjacent next round.
            spawn = spawn_strike_force(client, target)
        client.expect_ok("end_turn", 1)
    enemy_id, ecities, edead, won = enemy_state(client, my_id)
    assert not ecities, "enemy still holds a city after 30 assault rounds"
    print("  enemy cityless")

    # Hunt remaining enemy units (elimination may require it), watching for
    # the ending to be recognized.
    for rnd in range(25):
        enemy_id, ecities, edead, won = enemy_state(client, my_id)
        if edead or won:
            break
        units = [u for u in client.result("query_units")["units"]
                 if u.get("owner") == enemy_id]
        if units:
            upos = (units[0]["pos"]["x"], units[0]["pos"]["y"])
            army = army_at(client, spawn)
            if army is None:
                spawn = spawn_strike_force(client, upos)
                army = army_at(client, spawn)
            if cheb(spawn, upos) == 1:
                client.command("attack", army["index"], *upos)
            else:
                spawn = spawn_strike_force(client, upos)
                army = army_at(client, spawn)
                client.command("attack", army["index"], *upos)
        client.expect_ok("end_turn", 1)

    enemy_id, ecities, edead, won = enemy_state(client, my_id)
    assert edead or won, (
        f"ending never recognized: enemy dead={edead}, has_won={won}, "
        f"enemy cities={len(ecities)}"
    )
    print(f"  ending recognized: enemy dead={edead}, has_won={won}")

    # The endgame must not wedge the engine: more rounds + queries work.
    client.expect_ok("end_turn", 3)
    client.expect_ok("query_players")
    client.expect_ok("query_cities")
    print("  post-victory rounds + queries ok")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_scenario_victory.log"
    print(f"[victory] headless: {args.binary} (game log -> {log})")
    try:
        # players=3 -> barbarians(0) + human + ONE ai opponent.
        with Ctp2Client(args.binary, "headless", players=3, seed=42,
                        timeout=240, log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[victory] FAIL: {e}")
        print(f"[victory] see game log: {log}")
        return 1
    print("[victory] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
