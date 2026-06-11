#!/usr/bin/env python3
"""
One-command crash/behavior repro harness.

Boots a serve-mode binary, optionally loads a save (a path or a
test/fixtures name), runs a semicolon-separated command script, and
prints every exchange. If the game dies, the tail of the game log —
which now contains the crash handler's backtrace and recent-event
ring — is printed, so most crashes never need a debugger.

Examples:
    make repro CMDS='end_turn 5; query_players'
    make repro SAVE=c7-b-r80 CMDS='move_army 0 36 90; end_turn 4'
    make repro BIN=build-asan/ctp2_headless SAVE=/tmp/my.json CMDS='end_turn 1'

Exit code: 0 = all commands answered, 1 = a command failed or the game died.
"""

import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error, fixture_save  # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("binary")
    ap.add_argument("--save", default=None,
                    help="save to load first: a path, or a test/fixtures name")
    ap.add_argument("--cmds", required=True,
                    help="semicolon-separated verbs, e.g. 'move_army 0 36 90; end_turn 4'")
    ap.add_argument("--args", default="--players 3 --seed 42",
                    help="binary boot args (default: '--players 3 --seed 42')")
    ap.add_argument("--log", default="/tmp/ctp2_repro.log")
    args = ap.parse_args()

    boot = args.args.split()
    players = int(boot[boot.index("--players") + 1]) if "--players" in boot else 4
    seed = int(boot[boot.index("--seed") + 1]) if "--seed" in boot else 42

    save = args.save
    if save and not os.path.exists(save) and not save.startswith("/"):
        save = fixture_save(save)

    print(f"[repro] {args.binary} --players {players} --seed {seed} "
          f"(game log -> {args.log})")
    failed = False
    try:
        with Ctp2Client(args.binary, "headless", log_path=args.log,
                        players=players, seed=seed) as client:
            script = []
            if save:
                script.append(f"load_game {save}")
            script += [c.strip() for c in args.cmds.split(";") if c.strip()]
            for line in script:
                r = client.command(*line.split())
                status = r.get("status")
                print(f"  > {line}\n  < {json.dumps(r)[:300]}")
                if status != "ok":
                    failed = True
    except (Ctp2Error, OSError) as e:
        print(f"[repro] GAME DIED: {e}")
        print(f"[repro] ---- tail of {args.log} (crash report below) ----")
        try:
            with open(args.log) as f:
                print("".join(f.readlines()[-60:]))
        except OSError:
            pass
        crash = "/tmp/ctp2-crash.log"
        if os.path.exists(crash):
            print(f"[repro] ---- {crash} ----")
            with open(crash) as f:
                print("".join(f.readlines()[-50:]))
        return 1

    print("[repro] " + ("FAIL (a command errored)" if failed else "PASS"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
