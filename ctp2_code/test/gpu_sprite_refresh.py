#!/usr/bin/env python3
"""Units must refresh on the GPU without a camera/terrain refresh hiding bugs."""

import os
from pathlib import Path
import sys

from ctp2_client import Ctp2Client
from gpu_world_fallbacks import _bmp_dims, _bmp_pixel


def run(binary):
    artifacts = Path(binary).resolve().parent / "gpu-sprite-refresh"
    artifacts.mkdir(exist_ok=True)
    env = dict(os.environ, CTP2_GPU_LAYERS="1", CTP2_GPU_CAMERA="1",
               CTP2_GPU_QUADS="1", CTP2_GPU_WORLDMAP="1", CTP2_GPU_RASTER="1",
               CTP2_MODERN_SPRITES="1")
    socket = f"/tmp/ctp2-sprite-refresh-{os.getpid()}.sock"
    env["CTP2_SMOKE_SOCKET"] = socket
    with Ctp2Client(str(Path(binary).resolve()), "ui", env=env, socket_path=socket,
                    log_path=str(artifacts / "game.log")) as client:
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded()
        city = client.result("build_city")["pos"]
        client.expect_ok("debug_close_build_manager")
        client.expect_ok("debug_deselect")
        client.expect_ok("set_show_city_names", 0)
        client.expect_ok("debug_reveal_patch", city["x"], city["y"], 20)
        client.expect_ok("debug_clear_terrain_layers", city["x"], city["y"], 60)
        world = client.result("query_map")
        city_tile = next(t for t in world["tiles"] if (t["x"], t["y"]) == (city["x"], city["y"]))
        destination = {"x": (city["x"] + 1) % world["width"], "y": city["y"]}
        client.expect_ok("debug_set_terrain", destination["x"], destination["y"], city_tile["terrain"])
        sea = {"x": (city["x"] - 1) % world["width"], "y": city["y"]}
        shelf = next(t for t in client.result("query_terrains")["terrains"] if t["internal"] == "TERRAIN_WATER_SHALLOW")
        client.expect_ok("debug_set_terrain", sea["x"], sea["y"], shelf["id"])
        for army in reversed(client.result("query_armies")["armies"]):
            if army["pos"] == city:
                client.expect_ok("disband_unit", army["index"])
        client.expect_ok("camera_debug_center", city["x"], city["y"])

        def capture(name):
            path = artifacts / f"{name}.bmp"
            client.expect_ok("screenshot_presented", str(path))
            return path.read_bytes()

        baseline = client.result("query_gpu_world")
        assert baseline["worldmap"] and baseline["worldmap_texture"]
        before = capture("city")
        created = client.result("create_unit", "UNIT_ARCHER", city["x"], city["y"])
        # No centering, Refresh, or render-fixture command from here onwards.
        client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] > baseline["sprite_quads"],
                          timeout=5, desc="unit in city appears without terrain refresh")
        width, height = _bmp_dims(before)
        # New actors fade in on the animation clock. Submission can precede
        # visible pixels, so wait for the presented result, not just a quad.
        original_pixels = []
        after = before

        def creation_visible():
            nonlocal after, original_pixels
            after = capture("city-with-unit")
            original_pixels = [(x, y) for y in range(40, height - 200)
                               for x in range(300, width - 20)
                               if _bmp_pixel(before, x, y) != _bmp_pixel(after, x, y)]
            return len(original_pixels) > 20

        client.wait_until(creation_visible, timeout=5, desc="created unit produces visible pixels")
        changed = len(original_pixels)
        assert changed > 20, f"unit produced no visible map pixels ({changed})"
        print(f"PASS: unit in city appears with a stationary camera ({changed} pixels)", flush=True)

        armies = client.result("query_armies")["armies"]
        army = next(a for a in armies if a["pos"] == city
                    and any(u["type"] == created["type"] for u in a["units"]))
        client.expect_ok("debug_actor_state", army["index"], 0, "select")
        selected = capture("selected-unit")
        assert selected != after, "selection produced no visual change"
        client.expect_ok("debug_actor_state", army["index"], 0, "hidden")
        client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] == baseline["sprite_quads"],
                          timeout=5, desc="unit losing visibility disappears")
        hidden = capture("hidden-unit")
        assert hidden != selected, "visibility loss left stale pixels"
        client.expect_ok("debug_actor_state", army["index"], 0, "visible")
        client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] > baseline["sprite_quads"],
                          timeout=5, desc="unit regaining visibility appears")
        capture("visible-unit")
        print("PASS: selection and visibility transitions refresh without camera movement", flush=True)

        warrior = client.result("create_unit", "UNIT_WARRIOR", city["x"], city["y"])
        warrior_army = next(a for a in client.result("query_armies")["armies"]
                            if a["pos"] == city and any(u["type"] == warrior["type"] for u in a["units"]))
        client.expect_ok("debug_actor_state", warrior_army["index"], 0, "select")
        warrior_frame = capture("stack-warrior-selected")
        client.expect_ok("debug_actor_state", army["index"], 0, "select")
        archer_frame = capture("stack-archer-selected")
        assert sum(_bmp_pixel(warrior_frame, x, y) != _bmp_pixel(archer_frame, x, y)
                   for x, y in original_pixels) > 20, "stack selection kept the previous sprite"
        client.expect_ok("disband_unit", warrior_army["index"])
        print("PASS: switching the selected army changes the visible stack actor", flush=True)

        moved = client.result("move_army", army["index"], destination["x"], destination["y"])
        assert moved["arrived"], moved

        def movement_visible():
            frame = capture("unit-moved")
            # Movement must clear a substantial part of the original unit image.
            return sum(_bmp_pixel(frame, x, y) == _bmp_pixel(before, x, y)
                       for x, y in original_pixels) > len(original_pixels) // 2

        client.wait_until(movement_visible, timeout=5, desc="moving unit clears its old position")
        moved_frame = capture("unit-moved")
        assert client.result("query_gpu_world")["sprite_quads"] > baseline["sprite_quads"]
        print("PASS: movement updates the sprite position with a stationary camera", flush=True)
        client.expect_ok("disband_unit", army["index"])
        client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] == baseline["sprite_quads"],
                          timeout=5, desc="disbanded unit disappears without terrain refresh")
        removed = capture("city-after-disband")
        restored = sum(_bmp_pixel(moved_frame, x, y) != _bmp_pixel(removed, x, y)
                       for y in range(40, height - 200) for x in range(300, width - 20))
        assert restored > 20, f"disband left stale unit pixels ({restored})"
        final = client.result("query_gpu_world")
        assert final["camera"] == baseline["camera"]
        assert final["worldmap_origin"] == baseline["worldmap_origin"]
        # "complete" describes the older window-quad path; whole-map terrain
        # supports rivers even when that path reports terrain-rivers.
        assert not final["sprite_fallback_reason"], final["sprite_fallback_reason"]
        print("PASS: disband removes the sprite without camera movement", flush=True)

        # Save/restore must discard all texture references to the previous session.
        save = artifacts / "reload.sav"
        client.expect_ok("create_unit", "UNIT_ARCHER", city["x"], city["y"])
        restored_army = next(a for a in client.result("query_armies")["armies"]
                             if a["pos"] == city and any(u["type"] == created["type"] for u in a["units"]))
        client.expect_ok("debug_actor_state", restored_army["index"], 0, "select")
        client.expect_ok("save_game", save)
        for iteration in range(3):
            client.expect_ok("load_game", save)
            client.wait_game_loaded()
            client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] > baseline["sprite_quads"],
                              timeout=5, desc="sprites restored after session replacement")
            frame = capture(f"reload-{iteration}")
            assert any(_bmp_pixel(frame, x, y) != _bmp_pixel(before, x, y)
                       for x, y in original_pixels), "restored unit has no visible pixels"
        print("PASS: repeated save/load restores live sprites", flush=True)

        effect_before = capture("before-effect")
        count = client.result("query_gpu_world")["sprite_quads"]
        client.expect_ok("debug_combat_flash", destination["x"], destination["y"])
        client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] > count,
                          timeout=5, interval=0.01, desc="combat effect appears")
        effect = capture("combat-effect")
        assert effect != effect_before, "combat effect has no visible pixels"
        client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] == count,
                          timeout=10, desc="finished combat effect disappears")
        capture("after-effect")
        print("PASS: transient combat effect appears and expires", flush=True)

        archer = next(a for a in client.result("query_armies")["armies"]
                      if a["pos"] == city and any(u["type"] == created["type"] for u in a["units"]))
        boat = client.result("create_unit", "UNIT_LONGSHIP", sea["x"], sea["y"])
        client.expect_ok("debug_actor_state", archer["index"], 0, "select")
        before_board = capture("before-board")
        client.expect_ok("move_army", archer["index"], sea["x"], sea["y"])
        transport = next(a for a in client.result("query_armies")["armies"]
                         if a["pos"] == sea and any(u["type"] == boat["type"] for u in a["units"]))
        assert transport["cargo"], "boarding did not put the unit into cargo"
        def boarded_visible():
            frame = capture("aboard")
            return sum(_bmp_pixel(frame, x, y) != _bmp_pixel(before_board, x, y)
                       for x, y in original_pixels) > 20
        client.wait_until(boarded_visible, timeout=5, desc="embarked unit disappears from map")
        client.expect_ok("end_turn", 1)
        client.expect_ok("unload", transport["index"], city["x"], city["y"])
        client.expect_ok("end_turn", 1)
        client.wait_until(lambda: any(a["pos"] == city and any(u["type"] == created["type"] for u in a["units"])
                                     for a in client.result("query_armies")["armies"]),
                          timeout=5, desc="cargo returns ashore")
        capture("ashore")
        print("PASS: boarding and unloading update map actors", flush=True)
        ashore = next(a for a in client.result("query_armies")["armies"]
                      if a["pos"] == city and any(u["type"] == created["type"] for u in a["units"]))
        before_death = capture("before-death")
        living_quads = client.result("query_gpu_world")["sprite_quads"]
        client.expect_ok("debug_actor_state", ashore["index"], 0, "die")
        client.wait_until(lambda: client.result("query_gpu_world")["sprite_quads"] < living_quads,
                          timeout=10, desc="death animation removes its actor")
        dead = capture("after-death")
        assert dead != before_death, "death left stale pixels"
        print("PASS: normal death animation releases the map actor", flush=True)
    print(f"Artifacts: {artifacts}")


if __name__ == "__main__":
    run(sys.argv[1])
