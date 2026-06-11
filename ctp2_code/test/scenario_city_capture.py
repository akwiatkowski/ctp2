#!/usr/bin/env python3
"""
Scenario: the full city-capture arc on real campaign state.

The capture-brink fixture freezes the most expensive moment to reproduce:
a grouped human stack standing ADJACENT to an enemy city, contact made.
The first headless capture ever (campaign 6, Niani) crashed twice in the
CaptureCityEvent -> UI-selection chain; this replays that arc every CI
run, plus the at-war contracts of the war-depth verbs:

    load -> declare_war -> bombard (honest damage report)
         -> attack until the city falls -> the city joins OUR list
         -> rioting telemetry -> buy_production on the captured city
         -> propose_peace (the AI answers for real)

The fixture also regression-guards the continent-cache rebuild: loading
it and running AI turns evaluates transport goals (Agent::
EstimateTransportUtility), which dereferenced the never-rebuilt
m_land_next_too_water cache after a JSON load — SIGSEGV found while
building this very fixture.

Usage: test/scenario_city_capture.py <path-to-ctp2_headless>
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error, fixture_save  # noqa: E402

ENEMY = 2  # Yaa


def cheb(p, q):
    return max(abs(p[0] - q[0]), abs(p[1] - q[1]))


def enemy_city(client):
    for c in client.result("query_cities")["cities"]:
        if c["owner"] == ENEMY:
            return c
    return None


def stack_next_to(client, pos):
    """The biggest human army adjacent to pos."""
    best = None
    for a in client.result("query_armies")["armies"]:
        ap = (a["pos"]["x"], a["pos"]["y"])
        if cheb(ap, pos) == 1:
            if best is None or len(a["units"]) > len(best["units"]):
                best = a
    return best


def run(client):
    client.expect_ok("load_game", fixture_save("capture-brink"))

    city = enemy_city(client)
    assert city, "fixture must show an adjacent enemy city"
    cpos = (city["pos"]["x"], city["pos"]["y"])
    army = stack_next_to(client, cpos)
    assert army, f"fixture must have a stack adjacent to {city['name']}"
    print(f"  brink: {len(army['units'])} units at {army['pos']} "
          f"vs {city['name']} {cpos} (pop {city['population']})")

    # War on. (The fixture is saved pre-declaration so this path runs too.)
    r = client.command("declare_war", ENEMY)
    assert r.get("status") == "ok" or r.get("detail") == "already_at_war", (
        f"declare_war failed: {r}"
    )

    # Bombard the defenders: honest report — damage or an honest zero.
    r = client.command("bombard", army["index"], *cpos)
    if r.get("status") == "ok":
        res = r["result"]
        assert "damage_dealt" in res and "defenders_left" in res, f"bombard report incomplete: {r}"
        print(f"  bombard: damage {res['damage_dealt']:.1f}, "
              f"defenders left {res['defenders_left']}")
    else:
        # No bombard-capable unit in the stack is a legitimate refusal.
        assert r.get("detail") in ("cannot_bombard", "nothing_to_bombard"), (
            f"bombard refused for the wrong reason: {r}"
        )
        print(f"  bombard refused honestly: {r.get('detail')}")

    # Assault until the city falls. Indices shift as armies die — re-find
    # the adjacent stack each wave. This is the CaptureCityEvent path that
    # crashed campaign 6.
    captured = False
    for wave in range(12):
        a = stack_next_to(client, cpos)
        if a is None:
            break
        r = client.command("attack", a["index"], *cpos)
        assert r.get("status") == "ok", f"attack failed: {r}"
        own = [c for c in client.result("query_cities")["cities"]
               if (c["pos"]["x"], c["pos"]["y"]) == cpos and c["owner"] != ENEMY]
        if own:
            captured = True
            print(f"  CAPTURED {own[0]['name']} on wave {wave + 1}")
            break
        client.expect_ok("end_turn")
    assert captured, "the stack failed to take the city — fixture too weak?"

    # The captured city is OURS now: visible in our list, with riot
    # telemetry, and manageable.
    mine = [c for c in client.result("query_cities")["cities"]
            if (c["pos"]["x"], c["pos"]["y"]) == cpos]
    assert mine and mine[0]["owner"] != ENEMY, f"captured city not ours: {mine}"
    assert "rioting" in mine[0], f"captured city lacks riot telemetry: {mine[0]}"
    idx = mine[0]["index"]
    detail = client.result("query_city", idx)
    print(f"  captured city: pop {detail['population']}, rioting {detail['rioting']}, "
          f"{len(detail['buildable_buildings'])} buildable buildings")

    # buy_production on the captured city: honest either way.
    if not detail["building"] and detail["buildable_buildings"]:
        client.expect_ok("set_production", idx,
                         f"building {detail['buildable_buildings'][0]['type']}")
    r = client.command("buy_production", idx)
    assert (r.get("status") == "ok"
            or r.get("detail") in ("not_enough_gold", "already_bought",
                                   "nothing_being_built")), (
        f"buy_production on captured city: {r}"
    )
    print(f"  buy_production: {r.get('detail') or r['result']}")

    # Peace through the real diplomacy pipeline — the AI may refuse; the
    # verdict fields must be present and consistent either way.
    r = client.command("propose_peace", ENEMY)
    assert r.get("status") == "ok", f"propose_peace failed: {r}"
    res = r["result"]
    assert {"at_war", "peace_treaty", "accepted"} <= set(res), f"peace verdict incomplete: {res}"
    print(f"  propose_peace: accepted={res['accepted']} at_war={res['at_war']}")

    # The world survives the whole arc — run a few AI rounds on top (this
    # is also the transport-goal/continent-cache regression window).
    client.expect_ok("end_turn", 3)
    assert client.result("query_cities")["cities"], "world unqueryable after the arc"
    print("  post-arc rounds ran clean")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_capture_scenario.log"
    print(f"[capture] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, "headless", log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[capture] FAIL: {e}")
        print(f"[capture] see game log: {log}")
        return 1
    print("[capture] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
