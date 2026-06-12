#!/usr/bin/env python3
"""
Research + terraform slice — HEADLESS ONLY.

Research: query_research lists the affordance set; set_research switches the
goal; science accumulation completes an advance within a bounded number of
rounds (known_count grows).

Terraform: set_material_tax accrues Public Works; query_terraform honors the
gates (own territory + terrain add/remove advances — grant_advance debug verb
shortcuts the 200-round tech climb); terraform spends PW and the tile's
terrain actually changes after the option's turn count.

Usage:
    test/research.py <path-to-ctp2_headless> [--log FILE]
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402


def run(client):
    client.expect_ok("new_game")
    client.expect_ok("start_game")
    client.wait_game_loaded()
    client.expect_ok("build_city")

    # --- research ---
    r = client.result("query_research")
    known0 = r["known_count"]
    assert r["available"], f"no researchable advances at game start: {r}"
    pick = min(r["available"], key=lambda a: a["cost"])
    got = client.result("set_research", pick["id"])
    assert got["id"] == pick["id"], f"set_research mismatch: {got}"
    print(f"  researching {got['name']} (cost {pick['cost']})")

    for _ in range(8):
        client.expect_ok("end_turn", 10)
        if client.result("query_research")["known_count"] > known0:
            break
    known1 = client.result("query_research")["known_count"]
    assert known1 > known0, f"no advance completed in 80 rounds ({known0} -> {known1})"
    print(f"  advances: {known0} -> {known1}")

    # bad args stay errors
    assert client.command("set_research", 99999).get("detail") == "bad_advance"

    # --- terraform ---
    client.expect_ok("set_material_tax", 40)
    for aid in range(0, 110):  # debug shortcut past the tech climb
        client.command("grant_advance", aid)

    # query_terraform lists ALL buildable tile improvements (farm/mine/road/
    # structure/terraform); this slice exercises the terraform CLASS, so filter
    # to is_terraform options (the others lack to_terrain).
    def terraform_opts(tf):
        return [o for o in tf["options"] if o.get("is_terraform")]

    home = client.result("query_cities")["cities"][0]["pos"]
    spot = None
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            if dx == dy == 0:
                continue
            tf = client.result("query_terraform", home["x"] + dx, home["y"] + dy)
            if tf["tile_owner"] == 1 and terraform_opts(tf):
                spot = (home["x"] + dx, home["y"] + dy)
                break
        if spot:
            break
    assert spot, "no terraform options inside own borders with full tech tree"
    x, y = spot

    cheapest = min(terraform_opts(client.result("query_terraform", x, y)),
                   key=lambda o: o["cost"])
    print(f"  terraform target: {cheapest['name']} -> {cheapest['to_terrain_name']} "
          f"(cost {cheapest['cost']}, turns {cheapest['turns']})")

    for _ in range(30):
        tf = client.result("query_terraform", x, y)
        if tf["materials"] >= cheapest["cost"]:
            break
        client.expect_ok("end_turn", 10)
    assert tf["materials"] >= cheapest["cost"], f"PW never accrued: {tf['materials']}"

    before = tf["terrain"]
    client.expect_ok("terraform", x, y, cheapest["improvement_id"])
    for _ in range(cheapest["turns"] + 4):
        client.expect_ok("end_turn")
    after = next(t for t in client.result("query_map")["tiles"]
                 if (t["x"], t["y"]) == (x, y))
    assert after["terrain"] == cheapest["to_terrain"], (
        f"terrain did not transform: {before} -> {after['terrain']}"
    )
    print(f"  terrain {before} -> {after['terrain']} at ({x},{y}) — transformed")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_research_headless.log"
    print(f"[research] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, "headless", log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[research] FAIL: {e}")
        print(f"[research] see game log: {log}")
        return 1
    print("[research] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
