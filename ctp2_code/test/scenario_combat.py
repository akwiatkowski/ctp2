#!/usr/bin/env python3
"""
Scenario: combat invariant matrix — repeated real battles, engine-level checks.

scenario_city_capture covers ONE assault arc (stack -> adjacent city). This
scenario drives several battles on the same capture-brink fixture — bombard
then attack, round after round, against whatever enemy target is nearest
(field armies when visible, the city otherwise) — and after every action
asserts the invariants that must hold for ANY battle outcome (combat results
are RNG; exact outcomes are deliberately not asserted):

  - the verb responds ok or refuses honestly (no crash, no silent hang),
  - every surviving friendly unit has finite hp > 0,
  - our total unit count never INCREASES from fighting,
  - defenders_left reported by attack/bombard is >= 0,
  - the game keeps answering queries after each battle.

This exercises CTP2Combat (recently converted to vector<vector>) far more
than the single capture arc: multiple army compositions, bombard + melee,
counterattacks from AI turns in between.

Usage: test/scenario_combat.py <path-to-ctp2_headless>
"""

import argparse
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error, fixture_save  # noqa: E402

ENEMY = 2          # Yaa (same as scenario_city_capture)
MAX_ROUNDS = 10


def cheb(p, q):
    return max(abs(p[0] - q[0]), abs(p[1] - q[1]))


def my_armies(client):
    return client.result("query_armies")["armies"]


def check_army_hp(armies, when):
    total_units = 0
    for a in armies:
        for u in a["units"]:
            hp = u.get("hp")
            assert hp is not None and not (
                isinstance(hp, float) and (math.isnan(hp) or math.isinf(hp))
            ), f"{when}: unit with non-finite hp: {u}"
            assert hp > 0, f"{when}: surviving unit with hp<=0: {u}"
            total_units += 1
    return total_units


def enemy_targets(client):
    """Visible enemy units (field targets) + enemy cities, as positions."""
    units = client.result("query_units")["units"]
    field = [(u["pos"]["x"], u["pos"]["y"])
             for u in units if u.get("owner") == ENEMY]
    cities = [(c["pos"]["x"], c["pos"]["y"])
              for c in client.result("query_cities")["cities"]
              if c.get("owner") == ENEMY]
    return field, cities


def step_toward(client, army, target):
    ax, ay = army["pos"]["x"], army["pos"]["y"]
    nx = ax + (0 if target[0] == ax else (1 if target[0] > ax else -1))
    ny = ay + (0 if target[1] == ay else (1 if target[1] > ay else -1))
    client.command("move_army", army["index"], nx, ny)  # blocked moves ok


def run(client):
    client.expect_ok("load_game", fixture_save("capture-brink"))
    r = client.command("declare_war", ENEMY)
    assert r.get("status") == "ok" or "already" in str(r.get("detail", "")), (
        f"declare_war failed: {r}"
    )

    battles = 0
    for rnd in range(MAX_ROUNDS):
        armies = my_armies(client)
        assert armies, f"round {rnd}: no armies left at all"
        before_units = check_army_hp(armies, f"round {rnd} pre")

        field, cities = enemy_targets(client)
        targets = field + cities
        if not targets:
            print(f"  round {rnd}: no visible enemy targets left")
            break

        # The army closest to any target engages it (tie-break: bigger
        # stack). Picking by size alone can choose a distant army that
        # never arrives within MAX_ROUNDS.
        def closest(a):
            ap = (a["pos"]["x"], a["pos"]["y"])
            return (min(cheb(ap, t) for t in targets), -len(a["units"]))
        army = min(armies, key=closest)
        apos = (army["pos"]["x"], army["pos"]["y"])
        target = min(targets, key=lambda t: cheb(apos, t))
        kind = "field" if target in field else "city"

        if cheb(apos, target) > 1:
            step_toward(client, army, target)
        else:
            # Soften then strike. Both must answer ok or refuse honestly.
            rb = client.command("bombard", army["index"], *target)
            if rb.get("status") == "ok":
                res = rb["result"]
                assert res.get("defenders_left", 0) >= 0, f"bombard: {rb}"
            else:
                assert rb.get("detail") in (
                    "cannot_bombard", "nothing_to_bombard", "no_moves_left",
                    "not_at_war", "no_special_action",
                ), f"bombard refused for the wrong reason: {rb}"

            armies_now = my_armies(client)
            atk = next((a for a in armies_now
                        if (a["pos"]["x"], a["pos"]["y"]) == apos), None)
            if atk is not None:
                ra = client.command("attack", atk["index"], *target)
                if ra.get("status") == "ok":
                    battles += 1
                    left = ra["result"].get("defenders_left", 0)
                    assert left >= 0, f"attack: negative defenders_left: {ra}"
                    print(f"  round {rnd}: battle vs {kind} at {target}, "
                          f"defenders_left={left}")
                else:
                    # Honest refusals only (e.g. everything already dead
                    # from the bombard, or no movement points left).
                    assert ra.get("detail"), f"attack failed silently: {ra}"
                    print(f"  round {rnd}: attack refused: {ra.get('detail')}")

        # Post-action invariants: hp sane, no unit inflation from combat.
        after = check_army_hp(my_armies(client), f"round {rnd} post")
        assert after <= before_units, (
            f"round {rnd}: unit count grew from fighting: "
            f"{before_units} -> {after}"
        )

        client.expect_ok("end_turn", 1)   # AI turns: counterattacks happen

    assert battles >= 1, (
        "no battle ever resolved — fixture drifted or combat verbs broke"
    )
    check_army_hp(my_armies(client), "final")
    print(f"  done: {battles} battles resolved, invariants held")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_scenario_combat.log"
    print(f"[combat] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, "headless", log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[combat] FAIL: {e}")
        print(f"[combat] see game log: {log}")
        return 1
    print("[combat] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
