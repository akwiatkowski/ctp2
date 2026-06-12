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

Absorbed the former turns.py slice (same boot, one fewer game launch in
tier-B): end_turn advances full rounds through the real event pipeline,
query_turn tracks the clock exactly, the world stays consistent and
queryable afterwards, and bad arguments stay errors. UI build is excluded
on purpose: its end_turn queues director->AddEndTurn asynchronously, so
"advance N rounds synchronously" is a headless-only contract.

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

    # --- turn clock (absorbed turns.py) ---
    t0 = client.result("query_turn")
    assert t0["round"] == 0, f"fresh game should be at round 0, got {t0}"
    print(f"  start: round {t0['round']}, year {t0['year']}")

    client.expect_ok("build_city")
    home = client.result("query_cities")["cities"][0]
    hx, hy = home["pos"]["x"], home["pos"]["y"]
    print(f"  founded {home['name']} at ({hx},{hy})")

    # end_turn N advances exactly N full rounds; bare end_turn exactly one.
    r = client.command("end_turn", 3)
    assert r.get("status") == "ok", f"end_turn failed: {r}"
    t1 = client.result("query_turn")
    assert t1["round"] == 3, f"end_turn 3 should land on round 3, got {t1}"
    client.expect_ok("end_turn")
    t2 = client.result("query_turn")
    assert t2["round"] == 4, f"bare end_turn should advance 1, got {t2}"
    print(f"  clock ok: round 0 -> {t1['round']} -> {t2['round']}")

    # The world survived the rounds: players listed, the city exists and is
    # worked (yields reported), per-player queries agree with each other.
    players = client.result("query_players")["players"]
    assert any(p["human"] and not p["dead"] for p in players), "human player vanished"
    human = next(p for p in players if p["human"])
    detail = client.result("query_player", human["id"])
    assert detail["id"] == human["id"] and detail["num_cities"] >= 1, (
        f"query_player disagrees with query_players: {detail}"
    )
    mine = client.result("query_player_cities", human["id"])["cities"]
    assert mine and "yields" in mine[0], f"city yields missing: {mine}"
    y = mine[0]["yields"]
    print(f"  city yields after {t2['round']} rounds: food {y['food']}, "
          f"production {y['production']}, gold {y['gold']}, science {y['science']}")

    # bad args stay errors
    assert client.command("end_turn", 0).get("detail") == "bad_args"
    assert client.command("end_turn", 9999).get("detail") == "bad_args"

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

    # War-depth verb contracts (peacetime side — the at-war behavior runs on
    # the capture fixture in the scenario suite). All three must refuse
    # cleanly, never blind-ok.
    a = next(x for x in client.result("query_armies")["armies"] if x["index"] == idx)
    ax, ay = a["pos"]["x"], a["pos"]["y"]
    r = client.command("bombard", idx, ax + 1, ay)
    assert r.get("status") == "error" and r.get("detail") in (
        "nothing_to_bombard", "not_adjacent", "bad_position"), (
        f"bombard at empty/invalid tile must refuse: {r}"
    )
    assert client.command("bombard", idx).get("detail") == "bad_args"

    # No war in this slice: peace must be refused as not_at_war (and only
    # for real, contacted players).
    others = [p["id"] for p in client.result("query_players")["players"]
              if not p["human"] and not p["dead"]]
    if others:
        r = client.command("propose_peace", others[0])
        assert r.get("status") == "error" and r.get("detail") in (
            "not_at_war", "no_contact"), f"peacetime propose_peace must refuse: {r}"
    assert client.command("propose_peace", 99).get("detail") == "bad_player"

    # buy_production: honest either way — a real purchase reduces gold, an
    # unaffordable one reports the price. The military item set above may
    # still be in the queue; if not, queue another first.
    if not client.result("query_city", 0)["building"]:
        client.expect_ok("set_production", 0, "cheapest_military")
    r = client.command("buy_production", 0)
    if r.get("status") == "ok":
        assert r["result"]["cost"] >= 0 and "gold_after" in r["result"], (
            f"buy_production ok but no price/gold report: {r}"
        )
        print(f"  buy_production ok: paid {r['result']['cost']}")
    else:
        assert r.get("detail") in ("not_enough_gold", "already_bought"), (
            f"buy_production refused for the wrong reason: {r}"
        )
        print(f"  buy_production refused honestly: {r.get('detail')}")
    assert client.command("buy_production", 99).get("detail") == "bad_city_index"

    # --- efficiency/economy verbs added 2026-06-11 ----------------------------
    # query_city exposes buildable_wonders + gold_upkeep (NOT unit upkeep).
    city0 = client.result("query_city", 0)
    assert "buildable_wonders" in city0, "query_city missing buildable_wonders"
    assert "gold_upkeep" in city0, "query_city missing gold_upkeep"
    print(f"  query_city extras ok: {len(city0['buildable_wonders'])} wonders buildable, "
          f"gold_upkeep {city0['gold_upkeep']}")

    # set_production wonder: build an early wonder if any is available on a
    # fresh map, else the path must reject cleanly (bad_wonder / cannot_build).
    if city0["buildable_wonders"]:
        w = city0["buildable_wonders"][0]
        r = client.command("set_production", 0, "wonder", w["type"])
        assert r.get("status") == "ok" and r["result"]["name"] == w["name"], (
            f"set_production wonder failed: {r}"
        )
        print(f"  set_production wonder ok: {w['name']}")
    assert client.command("set_production", 0, "wonder", 99999).get("detail") == "bad_wonder"

    # set_government: bad id rejected; a high-tier gov needing an advance the
    # fresh civ lacks must report advance_missing (never crash / blind-ok).
    assert client.command("set_government", 99999).get("detail") == "bad_government"
    govr = client.command("set_government", 5)  # late gov => advance gate likely
    assert govr.get("status") in ("ok", "error"), f"set_government malformed: {govr}"
    if govr.get("status") == "error":
        assert govr.get("detail") in (
            "advance_missing", "already_that_government", "rejected"), (
            f"set_government refused for the wrong reason: {govr}")
    print(f"  set_government ok: {govr.get('result') or govr.get('detail')}")

    # establish_trade_route: bad indices rejected; and — the regression that
    # matters — an explicit good the source city cannot export must NOT crash
    # the engine (it used to SIGSEGV in CreateTradeRoute). Must reject, and the
    # engine must still answer afterwards.
    assert client.command("establish_trade_route", 99, 0).get("detail") == "bad_source_city"
    assert client.command("establish_trade_route", 0, 0).get("detail") == "same_city"
    r = client.command("establish_trade_route", 0, 0, 0)  # explicit good, same/no-good
    assert r.get("status") == "error", f"trade route should reject here: {r}"
    assert client.result("query_turn")["round"] >= 0, "engine died after trade route call"
    print(f"  trade route safe-reject ok: {r.get('detail')}")

    # disband_unit: bad index rejected; disbanding a real army removes it and
    # the engine stays consistent.
    assert client.command("disband_unit", 99999).get("detail") == "bad_army_index"
    before = client.result("query_armies")["armies"]
    if len(before) >= 2:
        victim = before[-1]["index"]
        r = client.command("disband_unit", victim)
        assert r.get("status") == "ok" and r["result"]["units_removed"] >= 1, (
            f"disband_unit failed: {r}")
        after = client.result("query_armies")["armies"]
        assert len(after) == len(before) - 1, "disband did not remove the army"
        print(f"  disband_unit ok: {len(before)} -> {len(after)} armies")

    # economy rate dials: science split + the workday/wages/rations sliders.
    # query_player exposes them; set_science_rate clamps to the government cap.
    econ = client.result("query_player", human["id"])["economy"]
    assert "science_rate" in econ and "rations" in econ, f"economy block missing: {econ}"
    print(f"  economy visible: science {econ['science_rate']:.2f} "
          f"(cap {econ['max_science_rate']:.2f}), rations level {econ['rations']['level']}")
    assert client.command("set_science_rate", 150).get("detail") == "out_of_range"
    r = client.command("set_science_rate", 40)
    assert r.get("status") == "ok" and r["result"]["science_rate"] <= econ["max_science_rate"] + 1e-9, (
        f"set_science_rate not clamped to government cap: {r}")
    print(f"  set_science_rate ok: applied {r['result']['science_rate']:.2f}")
    # lower rations to free food; -1 leaves the other two sliders alone.
    rexp = econ["rations"]["expectation"]
    r = client.command("set_rates", -1, -1, max(0, rexp - 1))
    assert r.get("status") == "ok" and "rations" in r["result"], f"set_rates failed: {r}"
    print(f"  set_rates ok: rations -> {r['result']['rations']['level']} "
          f"(expectation {r['result']['rations']['expectation']})")

    # military readiness footing: query_player.economy reports it; set_readiness
    # accepts peace/alert/war (and rejects junk) and applies immediately.
    assert "readiness" in econ and "level" in econ["readiness"], (
        f"economy.readiness missing: {econ}")
    assert client.command("set_readiness", "nonsense").get("detail") == "bad_args"
    r = client.command("set_readiness", "war")
    assert r.get("status") == "ok" and r["result"]["readiness"]["label"] == "war", (
        f"set_readiness war failed: {r}")
    r = client.command("set_readiness", "peace")
    assert r.get("status") == "ok" and r["result"]["readiness"]["level"] == 0, (
        f"set_readiness peace failed: {r}")
    print(f"  set_readiness ok: war -> peace, cost {r['result']['readiness']['cost']}")

    # specialists + governor: query_city exposes both; set_specialist validates
    # worker availability; set_governor toggles the mayor + profile.
    c0 = client.result("query_city", 0)
    assert "specialists" in c0 and "governor" in c0, f"city missing specialists/governor: {c0.keys()}"
    print(f"  specialists visible: {c0['specialists']} | governor {c0['governor']}")
    assert client.command("set_specialist", 0, 9, 1).get("detail") == "bad_pop_type"
    workers = c0["specialists"]["workers"]
    if workers >= 1:
        r = client.command("set_specialist", 0, 1, 1)  # +1 scientist
        assert r.get("status") == "ok" and r["result"]["now"] >= 1, f"set_specialist failed: {r}"
        print(f"  set_specialist ok: scientists -> {r['result']['now']}")
    else:
        assert client.command("set_specialist", 0, 1, 1).get("detail") == "not_enough_workers"
        print("  set_specialist guard ok: not_enough_workers")

    profs = client.result("query_governor_profiles")["profiles"]
    assert profs and all("index" in p and "name" in p for p in profs), f"no governor profiles: {profs}"
    r = client.command("set_governor", 0, 1, profs[0]["index"])  # raw socket: ints, not bools
    assert r.get("status") == "ok" and r["result"]["enabled"] is True, f"set_governor on failed: {r}"
    r2 = client.command("set_governor", 0, 0)
    assert r2.get("status") == "ok" and r2["result"]["enabled"] is False, f"set_governor off failed: {r2}"
    assert client.command("set_governor", 0, 1, 99999).get("detail") == "bad_build_list_sequence"
    print(f"  governor ok: {len(profs)} profiles, toggle on/off verified")

    # unit-order dispatcher: query the army's special orders, then exercise the
    # do_unit_order gate (must REPORT, never crash, on an illegal/empty target).
    mil = next((a for a in client.result("query_armies")["armies"]
                if any(u["name"] != "Settler" for u in a["units"])), None)
    if mil:
        uo = client.result("query_unit_orders", mil["index"])
        assert "orders" in uo, f"query_unit_orders missing orders: {uo}"
        print(f"  query_unit_orders ok: {len(uo['orders'])} orders for army {mil['index']}")
        assert client.command("do_unit_order", mil["index"], 99999).get("detail") == "bad_order_index"
        if uo["orders"]:
            oi = uo["orders"][0]["order_index"]
            ax, ay = mil["pos"]["x"], mil["pos"]["y"]
            # far tile => not_adjacent; engine never crashes.
            r = client.command("do_unit_order", mil["index"], oi, (ax + 5) % 60, ay)
            assert r.get("status") == "error", f"do_unit_order should reject far target: {r}"
            assert client.result("query_turn")["round"] >= 0, "engine died after do_unit_order"
            print(f"  do_unit_order safe-reject ok: {r.get('detail')}")
        # upgrade: either nothing_to_upgrade or a clean ok.
        r = client.command("upgrade_unit", mil["index"])
        assert r.get("status") == "ok" or r.get("detail") == "nothing_to_upgrade", f"upgrade_unit: {r}"
        print(f"  upgrade_unit ok: {r.get('result') or r.get('detail')}")

    # diplomacy proposal generalisation: bad player rejected up front; a real
    # proposal returns a verdict or a clean no_contact (early game), never crash.
    assert client.command("propose", 99, 33).get("detail") == "bad_player", "propose bad-player guard"
    if others:
        r = client.command("propose", others[0], 19, 50)  # offer 50 gold
        assert r.get("status") in ("ok", "error"), f"propose malformed: {r}"
        assert client.result("query_turn")["round"] >= 0, "engine died after propose"
        print(f"  propose ok: {r.get('result') or r.get('detail')}")

    # sell_building: bad building rejected; selling one the city HAS works.
    assert client.command("sell_building", 0, 99999).get("detail") == "bad_building"
    built = client.result("query_city", 0).get("buildings_built", [])
    if built:
        r = client.command("sell_building", 0, built[0]["type"])
        assert r.get("status") == "ok" and "gold" in r["result"], f"sell_building failed: {r}"
        print(f"  sell_building ok: sold {r['result']['name']}, gold {r['result']['gold']}")

    # trade routes: query never errors; cancel rejects bad indices safely.
    tr = client.result("query_trade_routes")
    assert "routes" in tr, f"query_trade_routes missing routes: {tr}"
    assert client.command("cancel_trade_route", 99, 0).get("detail") == "bad_city_index"
    assert client.command("cancel_trade_route", 0, 99999).get("detail") == "bad_route_index"
    print(f"  trade routes ok: {len(tr['routes'])} routes, cancel guards verified")


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
