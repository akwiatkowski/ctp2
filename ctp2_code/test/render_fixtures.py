"""Render-fixture commands place sprite quads with no game objects (ctp2-306).

Regression net for the render_* verbs: a fresh map plus two fixture sprites
must submit quads with no sprite fallback, and render_new_scene must drop
them again. Uses the gallery boot sequence (new_game + start_game).
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from ctp2_client import Ctp2Client  # noqa: E402


def main():
    binary = sys.argv[1]
    sock = f"/tmp/ctp2-render-fixtures-{os.getpid()}.sock"
    try:
        os.unlink(sock)
    except OSError:
        pass
    env = dict(os.environ, CTP2_SMOKE_SOCKET=sock, CTP2_GPU_LAYERS="1",
               CTP2_GPU_CAMERA="1", CTP2_GPU_QUADS="1", CTP2_GPU_WORLDMAP="1",
               CTP2_GPU_RASTER="1", CTP2_MODERN_SPRITES="1")
    with Ctp2Client(binary, "ui", seed=42, players=4,
                    socket_path=sock, env=env) as client:
        client.expect_ok("debug_set_good_richness", 50)
        client.expect_ok("new_game")
        client.expect_ok("start_game")
        client.wait_game_loaded(timeout=240)
        client.expect_ok("debug_reveal_patch", 30, 30, 10)

        base = client.result("query_gpu_world")
        base_quads = base.get("sprite_quads", 0)

        # Fixture sprites submit with no game objects and no fallback.
        r = client.result("render_new_scene")
        assert r.get("fixtures") == 0, r
        r = client.result("render_add_unit_sprite", "UNIT_MARINE", 30, 30,
                          "IDLE", 0, 0, 0)
        r = client.result("render_add_unit_sprite", "UNIT_ARCHER", 32, 30,
                          "ATTACK", 0, 4, 0)
        assert r.get("fixture") == 1, r
        state = client.result("query_gpu_world")
        assert not state.get("sprite_fallback_reason"), state
        assert state.get("sprite_quads", 0) == base_quads + 2, state

        # Bad inputs reject cleanly with no fixture recorded.
        bad = client.command("render_add_unit_sprite", "UNIT_NOPE", 30, 30)
        assert bad.get("status") == "error", bad
        bad = client.command("render_add_unit_sprite", "UNIT_MARINE", 30, 30,
                             "DANCE", 0, 0, 0)
        assert bad.get("status") == "error", bad

        # render_new_scene drops the placements again.
        client.expect_ok("render_new_scene")
        state = client.result("query_gpu_world")
        assert state.get("sprite_quads", -1) == base_quads, state

        # Tile + fog ops apply without gameplay verbs.
        r = client.result("render_set_tile", 33, 33, 0)

        assert r.get("terrain") == 0, r
        r = client.result("render_set_fog", 30, 30, 1)
        assert r.get("fogged") is True, r
        # Camera zoom matrix (ctp2-266): every layer renders at each zoom —
        # sprite quads and raster cells stay positive, no fixture fallback.
        for zoom in (0.5, 1.0, 1.5, 2.0):
            pose = client.result("render_camera_pose", 0, 0, zoom)
            cam = pose.get("camera", {})
            assert cam.get("zoom", 0) > 0, pose
            state = client.result("query_gpu_world")
            assert state.get("sprite_quads", 0) > 0, (zoom, state)
            build = state.get("last_terrain_build", {})
            assert build.get("gpu_raster", 0) > 0, (zoom, state)
            reason = state.get("sprite_fallback_reason", "")
            assert not reason.startswith("fixture-"), (zoom, state)

    print("PASS: render fixtures submit, reject, and clear", flush=True)


if __name__ == "__main__":
    main()
