#!/usr/bin/env python3
"""AI trial: very long all-AI game tracking tech progress per player.

Runs 8 AI players at high difficulty and prints a status table every
STATUS_EVERY seconds: round/year, per-player cities (count + best), units,
tech level (advance count + researching + last completed), wonders, score.

Goal: a tool proving the AI reaches and uses advanced tech. A tech stall
(level flat for hundreds of rounds while the game runs) is the failure mode.

Usage:
    make trial
    TRIAL_TURNS=100 TRIAL_PLAYERS=4 mise exec -- python3 ctp2_code/test/ai_trial.py

Env:
    TRIAL_BINARY      — game binary (default: build/ctp2_headless)
    TRIAL_TURNS       — rounds to run (default: 500)
    TRIAL_PLAYERS     — player slots (default: 8)
    TRIAL_SEED        — map seed (default: 42)
    TRIAL_DIFFICULTY  — 0-5, Deity=5 (default: 5)
    TRIAL_STATUS_EVERY— status cadence seconds (default: 10)
    TRIAL_BATCH       — rounds per end_turn call (default: 5)
    TRIAL_TIMEOUT     — seconds per end_turn batch (default: 240)
"""
import os
from pathlib import Path
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402

BINARY = os.environ.get("TRIAL_BINARY") or (
    sys.argv[1] if len(sys.argv) > 1 else None
) or os.path.join(os.path.dirname(__file__), "..", "..", "build", "ctp2_headless")
TURNS = int(os.environ.get("TRIAL_TURNS", "500"))
PLAYERS = int(os.environ.get("TRIAL_PLAYERS", "8"))
SEED = int(os.environ.get("TRIAL_SEED", "42"))
DIFFICULTY = int(os.environ.get("TRIAL_DIFFICULTY", "5"))
STATUS_EVERY = float(os.environ.get("TRIAL_STATUS_EVERY", "10"))
BATCH = int(os.environ.get("TRIAL_BATCH", "5"))
TIMEOUT = int(os.environ.get("TRIAL_TIMEOUT", "240"))


def fmt_year(year):
    return f"{-year}BC" if year is not None and year < 0 else str(year)


def status(client, turn, prev_adv, recent, t0):
    g = client.result("query_players")
    players = g["players"]
    cities = client.result("query_all_cities").get("cities", [])
    best = {}
    for c in cities:
        o, pop = c.get("owner"), c.get("population", 0) or 0
        if o not in best or pop > best[o][1]:
            best[o] = (c.get("name", "?"), pop)
    elapsed = time.time() - t0
    print(f"[trial] round {turn.get('round')} ({fmt_year(turn.get('year'))}) "
          f"t={elapsed:.0f}s")
    print(f"  {'id':>2} {'civ':<10} {'cities':>6} {'best city':<22} "
          f"{'units':>5} {'tech':>4} {'researching':<18} {'last':<18} "
          f"{'wond':>4} {'score':>7}")
    for p in players:
        pid = p["id"]
        adv = {a["id"]: a.get("name", "?") for a in p.get("advances", [])}
        new = sorted(set(adv) - prev_adv.get(pid, set()))
        prev_adv[pid] = set(adv)
        if new:
            recent[pid] = ",".join(adv[i] for i in new[:2])
        researching = (p.get("researching") or {}).get("name", "-")
        bc, bpop = best.get(pid, ("-", 0))
        alive = "" if p.get("dead") else "alive"
        print(f"  {pid:>2} {p.get('civ', '?'):<10} {p.get('num_cities', 0):>6} "
              f"{f'{bc}({bpop})':<22} {p.get('num_units', 0):>5} "
              f"{len(adv):>4} {researching:<18} {recent.get(pid, '-'):>18} "
              f"{p.get('num_wonders', 0):>4} {p.get('score', 0):>7} {alive}")
    totals = (
        sum(p.get("num_cities", 0) for p in players),
        sum(p.get("num_units", 0) for p in players),
        sum(p.get("num_wonders", 0) for p in players),
    )
    print(f"  totals: cities={totals[0]} units={totals[1]} wonders={totals[2]}")
    sys.stdout.flush()
    return players


def main():
    binary = str(Path(BINARY).resolve())
    sock = f"/tmp/ctp2-trial-{os.getpid()}.sock"
    env = dict(os.environ, CTP2_SMOKE_SOCKET=sock)
    log = Path(binary).parent / "ai-trial-game.log"
    print(f"[trial] {PLAYERS} AI @ difficulty {DIFFICULTY}, {TURNS} turns, "
          f"status every {STATUS_EVERY:g}s (game log -> {log})")
    t0 = time.time()
    prev_adv = {}
    recent = {}
    try:
        with Ctp2Client(binary, "headless", players=PLAYERS, seed=SEED,
                        timeout=TIMEOUT, socket_path=sock, env=env,
                        log_path=str(log)) as client:
            client.expect_ok("set_difficulty", DIFFICULTY)
            client.expect_ok("start_game")
            print(f"[trial] {client.expect_ok('enable_autoplay').get('detail')}")
            turn = client.result("query_turn")
            start_round = turn["round"]
            target = start_round + TURNS
            print(f"  start: round {start_round}")
            status(client, turn, prev_adv, recent, t0)
            next_status = t0 + STATUS_EVERY
            round_now = start_round
            stalls = 0
            while round_now < target:
                step = min(BATCH, target - round_now)
                try:
                    client.expect_ok("end_turn", step)
                except TimeoutError:
                    # Slow turns outrun the RPC timeout while the server keeps
                    # simulating: re-query and continue if the clock advanced.
                    stalled_at = round_now
                    round_now = client.result("query_turn")["round"]
                    if round_now > stalled_at:
                        stalls = 0
                        print(f"[trial] slow batch: reached round {round_now}, "
                              f"continuing")
                    else:
                        stalls += 1
                        print(f"[trial] stall {stalls}/3 at round {round_now}")
                        if stalls >= 3:
                            raise
                    continue
                round_now = client.result("query_turn")["round"]
                now = time.time()
                if now >= next_status or round_now >= target:
                    players = status(client, client.result("query_turn"),
                                     prev_adv, recent, t0)
                    next_status = now + STATUS_EVERY
                    if any(p.get("has_won_the_game") for p in players):
                        winner = next(p for p in players
                                      if p.get("has_won_the_game"))
                        print(f"[trial] won by player {winner.get('id')} "
                              f"({winner.get('civ')}) at round {round_now}")
                        break
            turn = client.result("query_turn")
            print(f"[trial] DONE round {turn['round']} "
                  f"({fmt_year(turn.get('year'))}) in {time.time() - t0:.0f}s")
    except (Ctp2Error, AssertionError, TimeoutError) as e:
        print(f"[trial] FAIL: {e}")
        print(f"[trial] see game log: {log}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
