#!/usr/bin/env python3
"""Run ten real frontend rounds, two starting settlers and Growth governors.

Usage: python3 ctp2_code/test/ui_ten_turns.py build/ctp2 [--seed 42]
No display/audio server or Pillow required. Normal in-memory rendering stays on;
passive frames, command trace, state snapshots and crash diagnostics are retained
in a unique ui-ten-turns-* directory beside the binary, even on failure.
"""
import argparse
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import uuid

from ctp2_client import Ctp2Client


NEW_GAME = "InitPlayWindow.NewGameButton"
LAUNCH = "SPNewGameWindow.StartButton"
NEXT_TURN = "ControlPanelWindow.ControlPanel.TurnButton"
ROUNDS = 10
# RC coordinates, not Cartesian tiles: see MapPoint::NormalizedSubtract.
NEIGHBORS = ((1, 0), (-1, 0), (0, 1), (-1, 1), (1, -1),
             (0, -1), (-1, 2), (1, -2))


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()
    assert 0 < args.seed <= 2147483647, "seed must be positive signed 32-bit"
    binary = args.binary.resolve()
    root = Path(__file__).resolve().parents[2]
    out = Path(tempfile.mkdtemp(prefix="ui-ten-turns-", dir=binary.parent))
    socket_path = f"/tmp/ctp2-ten-{uuid.uuid4().hex[:16]}.sock"

    # Ordinary New Game settings, never modified game data. Beginner supplies
    # the second starting settler (diffdb.txt EXTRA_SETTLER_CHANCE=1000000),
    # +40% human food and citysize0.txt's 75 growth coefficient. Disable huts so
    # population/unit gifts cannot masquerade as growth or settler production.
    settings = {
        "Difficulty": "0", "NumPlayers": "4", "NoGoodyHuts": "Yes",
        "AutoTurnCycle": "No", "AutoEndMultiple": "No",
        "AutoOpenCityWindow": "No", "CityBuiltMessage": "No",
        "MessageAdvice": "No", "TutorialAdvice": "No",
        "UnitCompleteMessages": "No", "NonContinuousUnitCompleteMessages": "No",
        "AutoSave": "No", "RunInBackground": "Yes",
        "XWrap": "Yes", "YWrap": "No",
    }
    profile = out / "profile.txt"
    lines = Path(__file__).with_name("testprofile.txt").read_text().splitlines()
    lines = [line for line in lines if line.split("=", 1)[0] not in settings]
    profile.write_text("\n".join(lines + [f"{k}={v}" for k, v in settings.items()]) + "\n")
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_VIDEODRIVER="dummy",
               SDL_AUDIO_DRIVER="dummy", SDL_AUDIODRIVER="dummy",
               SDL_RENDER_DRIVER="software", CTP2_CAPTURE_FRAMES="1",
               CTP2_PROFILE=str(profile), CTP2_SMOKE_SOCKET=socket_path)
    for key in env:
        if key.startswith("CTP2_GPU_") or key == "CTP2_MODERN_SPRITES":
            env[key] = ""
    trace = []
    step = "startup"
    last_command = None
    frame_sequence = 0
    print(f"Ten-round UI artifacts: {out}", flush=True)

    def record(event, **data):
        trace.append({"event": event, "elapsed": round(time.monotonic() - started, 3), **data})
        (out / "state.json").write_text(json.dumps(trace, indent=2) + "\n")

    started = time.monotonic()
    record("configuration", binary=str(binary), seed=args.seed, settings=settings,
           socket=socket_path, rounds=ROUNDS, turn_driver="real UI pointer; no RunRound")
    try:
        with Ctp2Client(str(binary), "ui", seed=args.seed, players=4,
                        cwd=str(root), env=env, socket_path=socket_path,
                        log_path=str(out / "game.log"), timeout=30) as client:
            def command(verb, *values, allow_error=False):
                nonlocal last_command
                last_command = " ".join(map(str, (verb, *values)))
                # Persist before the RPC: a blocked engine cannot acknowledge it.
                record("request", command=last_command)
                response = client.command(verb, *values)
                record("response", command=last_command, response=response)
                assert allow_error or response.get("status") == "ok", response
                return response

            def result(verb, *values):
                return command(verb, *values).get("result", {})

            def bounds(path):
                response = command("ui_control_bounds", path, allow_error=True)
                value = response.get("result", {})
                return value if (response.get("status") == "ok" and value.get("visible")
                                 and value.get("enabled") and value.get("width", 0) > 0
                                 and value.get("height", 0) > 0) else None

            def control(path):
                found = None

                def ready():
                    nonlocal found
                    found = bounds(path)
                    return found is not None

                client.wait_until(ready, timeout=30, interval=0.1, desc=path)
                return found

            def click_control(path):
                box = control(path)
                x, y = box["x"] + box["width"] // 2, box["y"] + box["height"] // 2
                for buttons in (0, 1, 0):
                    command("ui_pointer", x, y, buttons)

            def capture(name):
                nonlocal frame_sequence
                previous = frame_sequence
                info = {}

                def ready():
                    nonlocal info, frame_sequence
                    response = command("screenshot_frame", out / f"{name}.bmp", allow_error=True)
                    info = response.get("result", {})
                    if response.get("status") != "ok":
                        return False
                    frame_sequence = info["frame_sequence"]
                    return frame_sequence > previous

                client.wait_until(ready, timeout=30, interval=0.1, desc=f"normal frame {name}")
                record("frame", name=name, frame=info)

            def armies():
                return result("query_armies")["armies"]

            def own_cities():
                return result("query_player_cities", human)["cities"]

            def snapshot(name):
                state = {"turn": result("query_turn"), "armies": armies(),
                         "cities": [result("query_city", c["index"]) for c in own_cities()],
                         "units": [u for u in result("query_units")["units"] if u["owner"] == human]}
                record("snapshot", name=name, state=state)
                capture(name)
                return state

            def settler():
                matches = [a for a in armies() if any(u["id"] == second_unit for u in a["units"])]
                assert len(matches) == 1, f"starting settler {second_unit} lost/replaced: {matches}"
                assert matches[0]["can_settle"], matches
                return matches[0]

            def enable_growth(city):
                # Clear only the founder's default order. Never choose a build:
                # the subsequent empty->nonempty queue is the governor's choice.
                command("set_production", city["index"], "clear")
                command("set_governor", city["index"], 1, growth_profile["index"])
                baseline = result("query_city", city["index"])
                assert baseline["building"] is None, baseline
                assert baseline["governor"]["enabled"], baseline
                assert baseline["governor"]["build_list_sequence"] == growth_profile["index"], baseline
                baselines[city["id"]] = baseline
                histories[city["id"]] = [baseline]
                record("governor-baseline", city_id=city["id"], state=baseline)

            def distance(p, q):
                # The engine's normalized Manhattan distance in orthogonal XY.
                dx = abs(2 * (p["x"] - q["x"]) + p["y"] - q["y"]) % (2 * width)
                return (min(dx, 2 * width - dx) + abs(p["y"] - q["y"])) // 2

            try:
                step = "New Game / Launch"
                capture("main-menu")
                click_control(NEW_GAME)
                control(LAUNCH)
                command("ui_prepare_game", args.seed, 4)
                capture("setup")
                click_control(LAUNCH)
                client.wait_game_loaded(timeout=120)
                control(NEXT_TURN)
                humans = [p for p in result("query_players")["players"] if p["human"] and not p["dead"]]
                assert len(humans) == 1, humans
                human = humans[0]["id"]
                player = result("query_player", human)
                assert any(a["name"].casefold() == "agriculture" for a in player["advances"]), (
                    "the pinned start must unlock farmers legitimately", player["advances"])
                world = result("query_map")
                width, height = world["width"], world["height"]
                initial = snapshot("initial")
                start_round = initial["turn"]["round"]
                assert start_round == 0, initial["turn"]
                starting = [a for a in initial["armies"] if a["can_settle"]]
                assert len(starting) == 2 and all(len(a["units"]) == 1 for a in starting), starting
                assert not initial["cities"], initial["cities"]
                first_unit, second_unit = (a["units"][0]["id"] for a in starting)
                assert first_unit != second_unit
                initial_unit_ids = {u["id"] for a in initial["armies"] for u in a["units"]}
                profiles = result("query_governor_profiles")["profiles"]
                growth = [p for p in profiles if p["name"].casefold() == "growth"]
                assert len(growth) == 1, profiles
                growth_profile = growth[0]
                # Growth is the real build-list priority profile, not a trigger
                # on population increases. It may choose garrison before granary.
                record("starting-settlers", first=starting[0], second=starting[1], profile=growth_profile)
                baselines, histories, observed_outputs = {}, {}, []
                movement = [starting[1]["pos"]]
                step = "first starting settler founds city"
                command("build_city")
                cities = own_cities()
                assert len(cities) == 1 and cities[0]["pos"] == starting[0]["pos"], cities
                assert all(u["id"] != first_unit for a in armies() for u in a["units"]), "first settler survived founding"
                home = cities[0]
                enable_growth(home)
                settler()  # The other ORIGINAL settler, not a manufactured one.
                snapshot("first-city")
                second_city = None

                for completed in range(ROUNDS):
                    expected_round = start_round + completed
                    step = f"round {expected_round}: second settler orders"
                    assert result("query_turn")["round"] == expected_round, "unexpected automatic turn"
                    if second_city is None:
                        army = settler()
                        if distance(army["pos"], home["pos"]) >= 3 and army["moves_left"] > 0:
                            # build_city uses the first settler-capable army: do
                            # not accidentally found with a governor-built one.
                            first_available = next(a for a in armies() if a["can_settle"])
                            assert any(u["id"] == second_unit for u in first_available["units"]), first_available
                            command("build_city")
                            cities = own_cities()
                            assert len(cities) == 2, cities
                            second_city = next(c for c in cities if c["id"] != home["id"])
                            assert second_city["pos"] == army["pos"], (second_city, army)
                            assert all(u["id"] != second_unit for a in armies() for u in a["units"]), "second settler survived founding"
                            enable_growth(second_city)
                            record("second-city", city_id=second_city["id"], settler_id=second_unit,
                                   position=second_city["pos"], movement=movement)
                        elif army["moves_left"] > 0:
                            explored = {(t["x"], t["y"]) for t in result("query_map")["tiles"]}
                            candidates = [{"x": (army["pos"]["x"] + dx) % width,
                                           "y": army["pos"]["y"] + dy} for dx, dy in NEIGHBORS]
                            candidates = [p for p in candidates if 0 <= p["y"] < height
                                          and (p["x"], p["y"]) in explored
                                          and distance(p, home["pos"]) > distance(army["pos"], home["pos"])]
                            candidates.sort(key=lambda p: (-distance(p, home["pos"]), p["y"], p["x"]))
                            for target in candidates:
                                response = command("move_army", army["index"], target["x"], target["y"], allow_error=True)
                                if response.get("status") == "ok":
                                    client.wait_until(lambda: settler()["pos"] != army["pos"], timeout=15,
                                                      interval=0.1, desc="original second settler moves")
                                    moved = settler()
                                    assert moved["pos"] == target, (target, moved)
                                    movement.append(moved["pos"])
                                    record("settler-moved", unit_id=second_unit, before=army, after=moved)
                                    break
                            else:
                                raise AssertionError(f"no outward legal step for original settler: {army}; {candidates}")

                    # Seed 42's ordinary workers yield only 4 net food (300
                    # growth/turn). Employ a real existing citizen as the
                    # Agriculture-unlocked farmer until the city grows.
                    # Pop.txt supplies 30 food; the free city center still
                    # produces shields. The mayor may reassign citizens at
                    # FinishBeginTurn, so reapply this normal player choice,
                    # never a food/population grant or a production choice.
                    step = f"round {expected_round}: farming allocation"
                    for city in own_cities():
                        detail = result("query_city", city["index"])
                        if detail["population"] <= baselines[city["id"]]["population"]:
                            if detail["specialists"]["farmers"] == 0:
                                command("set_specialist", city["index"], 3, 1)
                            assigned = result("query_city", city["index"])
                            assert assigned["specialists"]["farmers"] >= 1, assigned
                            record("farmer-allocation", round=expected_round,
                                   city_id=city["id"], state=assigned)

                    step = f"complete frontend round {expected_round + 1}"
                    # No `end_turn` command (bare or numbered): DispatchSafe
                    # catches both and runs RunRound, bypassing the frontend.
                    click_control(NEXT_TURN)

                    def next_round():
                        turn = result("query_turn")
                        assert turn["round"] <= expected_round + 1, turn
                        return turn["round"] == expected_round + 1 and bounds(NEXT_TURN) is not None

                    client.wait_until(next_round, timeout=90, interval=0.25,
                                      desc=f"frontend completes exactly round {expected_round + 1}")
                    state = snapshot(f"round-{completed + 1:02d}")
                    assert state["turn"]["round"] == expected_round + 1, state["turn"]
                    for city in state["cities"]:
                        assert city["id"] in histories, f"unexpected city: {city}"
                        assert city["governor"]["enabled"] and city["governor"]["build_list_sequence"] == growth_profile["index"], city
                        histories[city["id"]].append(city)
                    observed_outputs.extend(u for u in state["units"] if not u["is_city"] and u["id"] not in initial_unit_ids)
                    print(f"Round {completed + 1}/{ROUNDS}: " + ", ".join(
                        f"{c['name']} pop={c['population']} shields={c['shields_stored']} queue={c['building']}"
                        for c in state["cities"]), flush=True)

                step = "ten-round outcome assertions"
                assert second_city is not None and len(state["cities"]) == 2, "second starting settler never founded"
                assert len({(p["x"], p["y"]) for p in movement}) >= 2, movement
                assert result("query_turn")["round"] == start_round + ROUNDS
                grown = [city_id for city_id, history in histories.items()
                         if any(c["population"] > baselines[city_id]["population"] for c in history)]
                assert grown, "no real integer population growth within ten rounds; see food/growth snapshots"
                for city_id in grown:
                    after_growth = [c for c in histories[city_id]
                                    if c["population"] > baselines[city_id]["population"]]
                    assert any(b["shields_stored"] > a["shields_stored"]
                               or b["building"] != a["building"]
                               for a, b in zip(after_growth, after_growth[1:])), (
                        f"automatic production did not continue after growth in city {city_id}")
                evidence = {}
                for city_id, history in histories.items():
                    chosen = [c["building"] for c in history if c["building"] is not None]
                    assert chosen, f"Growth governor never chose production for city {city_id}"
                    progressed = any(b["building"] is not None and a["building"] == b["building"]
                                     and b["shields_stored"] > a["shields_stored"]
                                     for a, b in zip(history, history[1:]))
                    new_buildings = {b["type"] for c in history for b in c["buildings_built"]} - {
                        b["type"] for b in baselines[city_id]["buildings_built"]}
                    outputs = [u for u in observed_outputs if u["pos"] == baselines[city_id]["pos"]
                               and any(b["type"] == u["type"] and b["category"] == 1 for b in chosen)]
                    assert progressed or new_buildings or outputs, f"governor queue stalled in city {city_id}"
                    evidence[city_id] = {"chosen": chosen, "shield_progress": progressed,
                                         "completed_buildings": sorted(new_buildings), "produced_units": outputs}
                record("passed", completed_rounds=ROUNDS, city_ids=list(histories),
                       starting_settler_ids=[first_unit, second_unit], grew_city_ids=grown,
                       production=evidence, movement=movement)
            except BaseException as error:
                record("failed", during=step, error=repr(error), last_command=last_command,
                       pid=client.proc.pid, exit_code=client.proc.poll())
                # Do not send more RPCs after a receive timeout: its late reply
                # could be mistaken for the next command's response.
                if client.proc.poll() is None and sys.platform == "darwin":
                    try:
                        subprocess.run(["/usr/bin/sample", str(client.proc.pid), "2", "-file",
                                        str(out / "hang.sample.txt")], timeout=10,
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
                    except (OSError, subprocess.TimeoutExpired) as diagnostic_error:
                        record("sample-failed", error=repr(diagnostic_error))
                if client.proc.poll() is None and not isinstance(error, (TimeoutError, socket.timeout)):
                    try:
                        client.sock.settimeout(2)
                        record("failure-frame", response=client.command("screenshot_frame", out / "failure.bmp"))
                    except Exception as diagnostic_error:
                        record("failure-frame-failed", error=repr(diagnostic_error))
                client.sock.settimeout(1)
                raise
    except BaseException:
        print(f"FAIL during {step}; see {out / 'state.json'} and {out / 'game.log'}", file=sys.stderr, flush=True)
        raise
    finally:
        Path(socket_path).unlink(missing_ok=True)
    print(f"PASS: two original settlers founded cities, Growth production and population growth, exactly ten UI rounds; {out}")


if __name__ == "__main__":
    run()
