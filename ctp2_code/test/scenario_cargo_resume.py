#!/usr/bin/env python3
"""
Scenario: a transport's queued sea path resumes across turns — with cargo.

The cargo-brink fixture holds a coracle CARRYING A BOARDED ARMY with a
multi-tile sea path already queued. Campaign 7's known issue was queued
paths dying after their first leg under the serve-mode round pump; the
fix (re-firing BeginTurnExecute per army) explicitly EXCLUDES cargo, so
the transport itself must keep its order alive while carrying.

    load -> end_turn x4 with NO re-issued orders
         -> the transport makes progress, the cargo stays aboard
         -> unload onto adjacent land once close enough (best effort).

Usage: test/scenario_cargo_resume.py <path-to-ctp2_headless>
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error, fixture_save  # noqa: E402


def transport(client):
    for a in client.result("query_armies")["armies"]:
        if a.get("cargo_capacity", 0) > 0:
            return a
    return None


def run(client):
    client.expect_ok("load_game", fixture_save("cargo-brink"))

    t = transport(client)
    assert t, "fixture must contain a transport army"
    assert t["cargo"], f"fixture transport must carry cargo: {t}"
    start = (t["pos"]["x"], t["pos"]["y"])
    ncargo = len(t["cargo"])
    print(f"  brink: transport {t['index']} at {start} carrying {ncargo}")

    # The queued path must survive the load AND the round pump: progress
    # with NO new orders issued.
    positions = [start]
    for _ in range(4):
        client.expect_ok("end_turn")
        t = transport(client)
        assert t, "transport vanished while sailing"
        assert len(t["cargo"]) == ncargo, (
            f"cargo lost at sea: {len(t['cargo'])} of {ncargo} left: {t}"
        )
        positions.append((t["pos"]["x"], t["pos"]["y"]))
    distinct = len(set(positions))
    assert distinct >= 2, (
        f"queued sea path did not resume after load: stuck at {positions}"
    )
    print(f"  sailed {positions[0]} -> {positions[-1]} "
          f"({distinct} distinct positions), cargo intact")

    # Best-effort amphibious finish: if any explored LAND tile is adjacent,
    # unload onto it and confirm the cargo comes ashore as an army.
    terr = {t["id"]: t for t in client.result("query_terrains")["terrains"]}
    land = {(x["x"], x["y"])
            for x in client.result("query_map")["tiles"]
            if terr.get(x["terrain"], {}).get("land")}
    cur = positions[-1]
    adj_land = [p for p in land
                if max(abs(p[0] - cur[0]), abs(p[1] - cur[1])) == 1]
    if adj_land:
        before = len(client.result("query_armies")["armies"])
        r = client.command("unload", transport(client)["index"], *adj_land[0])
        assert r.get("status") == "ok", f"unload failed: {r}"
        client.expect_ok("end_turn")
        after = client.result("query_armies")["armies"]
        assert len(after) > before or not transport(client)["cargo"], (
            "unload reported ok but nothing came ashore"
        )
        print(f"  unloaded onto {adj_land[0]} — cargo ashore")
    else:
        print("  no adjacent land in vision — sailing leg verified only")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_cargo_scenario.log"
    print(f"[cargo] headless: {args.binary} (game log -> {log})")
    try:
        with Ctp2Client(args.binary, "headless", log_path=log) as client:
            run(client)
    except (Ctp2Error, AssertionError) as e:
        print(f"[cargo] FAIL: {e}")
        print(f"[cargo] see game log: {log}")
        return 1
    print("[cargo] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
