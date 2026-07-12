#!/usr/bin/env python3
"""
Scenario: long-game soak — drive a fresh game deep into the late game and
assert the engine stays alive and self-consistent the whole way.

The fast integration tier only ever runs ~50-turn games, and the fixture
scenarios resume at round ~160. Nothing exercises the *engine* across the
full early→mid→late arc from a cold start: hundreds of rounds of AI turns,
tech accumulating deep into the tree, cities growing, armies fighting, the
map filling. That regime is exactly where a late-game-only regression from
the memory-safety refactors (RAII ownership, load-cache rebuilds, string
threading) would hide — a crash or corruption that a 50-turn game never
reaches.

This soak is a stability net, not a determinism check. It asserts what must
hold in ANY healthy game regardless of how the AI plays:

    start a new game -> run TARGET_ROUNDS rounds in batches -> each batch the
    clock advances by exactly the batch size (never stalls, never jumps) ->
    at the end at least one player is alive with cities, the game developed
    (cities were founded), and the per-city query walks cleanly (the
    UTF-8 city-name path — same crash surface scenario_load_stress guards).

A crash or heap corruption during the run kills the subprocess, so the next
RPC raises Ctp2Error and the scenario FAILs loudly with the game log.

Turns are chunked (BATCH rounds per end_turn) so no single RPC outlives the
socket timeout, and so a stall shows up at the batch it happened on.

Usage: test/scenario_long_game.py <path-to-ctp2_headless> [--rounds N]

Point it at build-ubsan/ctp2_headless for a stronger check — the same soak
then also traps undefined behaviour that only fires deep in the game.
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ctp2_client import Ctp2Client, Ctp2Error  # noqa: E402

# Round 300 is ~3x deeper than the fixture scenarios (round ~160) and reaches
# a developed mid/late game, while staying comfortably under the meson 600s
# timeout (per-turn cost grows as the game develops — a 500-round run measured
# ~6.5 min, so 300 leaves margin for slower CI hardware).
TARGET_ROUNDS = 300
BATCH = 50          # rounds per end_turn RPC (each well under the socket timeout)
PLAYERS = 6         # more AI players -> more late-game stress (combat, borders)


def _walk_numbers(obj, path=""):
    """Yield (path, value) for every number nested in a JSON-ish structure."""
    if isinstance(obj, dict):
        for k, v in obj.items():
            yield from _walk_numbers(v, f"{path}.{k}")
    elif isinstance(obj, list):
        for i, v in enumerate(obj):
            yield from _walk_numbers(v, f"{path}[{i}]")
    elif isinstance(obj, (int, float)) and not isinstance(obj, bool):
        yield path, obj


def check_economy_invariants(players, round_now):
    """Corruption catchers, not balance checks (#8). NaN/inf in economy
    floats, negative populations/city counts, or absurd gold magnitudes mean
    an uninitialised read or memory stomp — the memory-refactor regression
    class — long before they'd crash."""
    import math
    assert players, f"round {round_now}: query_players returned no players"
    for p in players:
        pid = p.get("id")
        for path, v in _walk_numbers(p):
            assert not (isinstance(v, float) and
                        (math.isnan(v) or math.isinf(v))), (
                f"round {round_now}: player {pid} has non-finite {path}={v}"
            )
        for key in ("num_cities", "num_units", "num_armies"):
            if key in p:
                assert 0 <= p[key] < 10000, (
                    f"round {round_now}: player {pid} {key}={p[key]} "
                    f"out of sane range"
                )
        if "gold" in p:
            # Deficits are legal (units disband), garbage is not.
            assert -1_000_000 < p["gold"] < 1_000_000_000, (
                f"round {round_now}: player {pid} gold={p['gold']} "
                f"looks like corruption"
            )


def run(client, target):
    client.expect_ok("start_game")
    round_now = client.result("query_turn")["round"]
    start_round = round_now
    print(f"  start: round {round_now}, {PLAYERS} players")

    while round_now < start_round + target:
        remaining = (start_round + target) - round_now
        step = min(BATCH, remaining)
        client.expect_ok("end_turn", step)
        prev = round_now
        round_now = client.result("query_turn")["round"]
        # The clock must advance by EXACTLY the batch size: a stall (0) or a
        # jump (SkipToRound stomping the global TurnCount, the load_game bug
        # class) both corrupt the round accounting.
        if round_now != prev + step:
            raise AssertionError(
                f"clock broken: round {prev} + {step} -> {round_now}"
            )
        print(f"  round {round_now}/{start_round + target}")

        # Economy sanity invariants (#8): not balance checks — the sim is
        # allowed to drift — but corruption catchers. A NaN/inf in the
        # economy floats, a negative population, or an absurd gold value
        # means memory corruption or an uninitialised read, exactly the
        # class the memory-safety refactors could regress.
        players = client.result("query_players")["players"]
        check_economy_invariants(players, round_now)

        # A conquest/score victory is a legitimate healthy finish — reaching it
        # without a crash is still a pass. Stop soaking rather than demanding
        # the full round count from an already-decided game.
        if any(p.get("has_won_the_game") for p in players):
            winner = next(p for p in players if p.get("has_won_the_game"))
            print(f"  game won by player {winner.get('id')} at round "
                  f"{round_now} — healthy early finish")
            break

    players = client.result("query_players")["players"]
    live = [p for p in players if not p.get("dead")]
    total_cities = sum(p.get("num_cities", 0) for p in players)
    assert live, "all players dead after a full soak — game collapsed"
    assert total_cities > 0, (
        "no cities exist after 300 rounds — the game never developed"
    )

    # Walk each live player's cities through the query path that dumps city
    # names as JSON (the UTF-8 codec surface that has bitten before). A
    # corrupt CityData or name buffer surfaces here rather than silently.
    for p in players:
        if p.get("dead") or p.get("num_cities", 0) == 0:
            continue
        idx = p.get("id")
        if idx is None:
            continue
        client.expect_ok("query_player_cities", idx)

    print(
        f"  end: round {round_now}, live={len(live)}/{len(players)}, "
        f"cities={total_cities}"
    )

    # Deep-state save/load (#3): the committed fixtures stop at round ~160,
    # so late-game serialization (bigger armies, deeper tech, more cities/
    # trade state) is otherwise never round-tripped. Save at the soak's end,
    # load it back, and prove the loaded game is alive: clock preserved,
    # same city total, and 20 more rounds run with exact clock advance.
    save_path = "/tmp/ctp2_longgame_deep.sav"
    client.expect_ok("save_game", save_path)
    client.expect_ok("load_game", save_path)
    r = client.result("query_turn")["round"]
    assert r == round_now, (
        f"deep save/load clock broken: saved at {round_now}, loaded {r}"
    )
    reloaded = client.result("query_players")["players"]
    reloaded_cities = sum(p.get("num_cities", 0) for p in reloaded)
    assert reloaded_cities == total_cities, (
        f"cities changed across deep save/load: {total_cities} -> {reloaded_cities}"
    )
    check_economy_invariants(reloaded, r)
    client.expect_ok("end_turn", 20)
    r2 = client.result("query_turn")["round"]
    assert r2 == r + 20, (
        f"clock broken after deep reload: {r} + 20 -> {r2}"
    )
    check_economy_invariants(client.result("query_players")["players"], r2)
    print(f"  deep save/load ok: round {r} preserved, +20 rounds -> {r2}")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary")
    ap.add_argument("--rounds", type=int, default=TARGET_ROUNDS)
    ap.add_argument("--log", default=None)
    args = ap.parse_args()
    log = args.log or "/tmp/ctp2_scenario_long_game.log"
    print(f"[long-game] headless: {args.binary} (game log -> {log})")
    try:
        # Deep-game turns run slower than the 120s default; give each batch
        # RPC generous headroom.
        with Ctp2Client(args.binary, "headless", players=PLAYERS, seed=42,
                        timeout=240, log_path=log) as client:
            run(client, args.rounds)
    except (Ctp2Error, AssertionError) as e:
        print(f"[long-game] FAIL: {e}")
        print(f"[long-game] see game log: {log}")
        return 1
    print("[long-game] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
