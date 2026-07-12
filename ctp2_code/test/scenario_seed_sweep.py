#!/usr/bin/env python3
"""
Scenario: seed sweep — the same short game across MANY rng seeds.

Until now the entire test suite lived on seed 42 (plus one 99 check): one
map, one set of starting positions, one AI trajectory. Map generation,
continent layout, start placement, and early-AI behavior across the seed
space were untested — a crash that needs an unlucky coastline shape would
never fire in CI.

Each seed runs a fresh headless game for --turns rounds via the plain CLI
(no socket) and must:
  - exit 0 (no crash, no sanitizer abort),
  - produce parseable metrics with >= 2 live players,
  - have founded at least one city by the end (the game DEVELOPED).

Usage:
  test/scenario_seed_sweep.py <binary> [--count N] [--turns T] [--players P]

Wiring:
  - meson `scenario-seed-sweep`: small config (3 seeds x 40 turns) on the
    debug binary — CI-affordable (~40s).
  - `make seed-sweep`: 20 seeds x 60 turns on build-release/ctp2_headless
    (~1 min thanks to the release tier) — run per milestone.
"""

import argparse
import os
import subprocess
import sys

# Fixed seed list head — deterministic sweep, reproducible failures.
# (Seed 42 is everywhere else already; start after it.)
SEED_BASE = 1000


def run_seed(binary, seed, turns, players, log_dir):
    log_path = os.path.join(log_dir, f"seed_{seed}.log")
    cmd = [
        binary, "--new-game",
        "--players", str(players),
        "--seed", str(seed),
        "--turns", str(turns),
        "--export-metrics", "-",
    ]
    with open(log_path, "w") as log:
        proc = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=log,
            timeout=600, text=True, errors="replace",
        )
    if proc.returncode != 0:
        raise AssertionError(
            f"seed {seed}: exited {proc.returncode} (log: {log_path})"
        )

    # Parse the "# PLAYERS" CSV section from stdout.
    lines = proc.stdout.splitlines()
    try:
        start = lines.index("# PLAYERS") + 2  # skip header row
    except ValueError:
        raise AssertionError(f"seed {seed}: no metrics in output (log: {log_path})")
    rows = []
    for line in lines[start:]:
        if not line.strip() or line.startswith("#"):
            break
        rows.append(line.split(","))
    assert rows, f"seed {seed}: empty player metrics"

    alive = [r for r in rows if r[2] == "no"]
    cities = sum(int(r[5]) for r in rows)
    assert len(alive) >= 2, (
        f"seed {seed}: only {len(alive)} players alive after {turns} turns"
    )
    assert cities >= 1, (
        f"seed {seed}: no cities founded in {turns} turns — game never developed"
    )
    return len(alive), cities


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--count", type=int, default=3, help="number of seeds")
    ap.add_argument("--turns", type=int, default=40)
    ap.add_argument("--players", type=int, default=4)
    ap.add_argument("--log-dir", default="/tmp/ctp2_seed_sweep")
    args = ap.parse_args()

    os.makedirs(args.log_dir, exist_ok=True)
    seeds = [SEED_BASE + i for i in range(args.count)]
    print(f"[seed-sweep] {args.binary}: seeds {seeds[0]}..{seeds[-1]}, "
          f"{args.turns} turns, {args.players} players")

    failures = []
    for seed in seeds:
        try:
            alive, cities = run_seed(args.binary, seed, args.turns,
                                     args.players, args.log_dir)
            print(f"  seed {seed}: ok (alive={alive}, cities={cities})")
        except (AssertionError, subprocess.TimeoutExpired) as e:
            print(f"  seed {seed}: FAIL — {e}")
            failures.append(seed)

    if failures:
        print(f"[seed-sweep] FAIL: seeds {failures}")
        return 1
    print(f"[seed-sweep] PASS ({len(seeds)} seeds)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
