#!/usr/bin/env python3
"""Smoke-test timelapse Chronicle caption rendering without running the game."""

from __future__ import annotations

import tempfile
from pathlib import Path

import render


def main() -> int:
    names = {
        "advances": {"7": "Bronze Working"},
        "buildings": {"3": "Granary"},
        "wonders": {"5": "Stonehenge"},
    }
    events = [
        {"event": "GrantAdvance", "player": 1, "args": [{"kind": "advance", "value": 7}]},
        {"event": "CreateBuilding", "player": 1, "args": [{"kind": "int", "value": 3}]},
        {"event": "CreateWonder", "player": 2, "args": [{"kind": "wonder", "value": 5}]},
        {"event": "MoveOrder", "player": 1, "args": []},  # intentionally ignored noise
    ]

    captions = render.frame_captions(events, names)
    rendered_text = "\n".join(text for _, text in captions)
    assert "Bronze Working" in rendered_text
    assert "Granary" in rendered_text
    assert "Stonehenge" in rendered_text
    assert "MoveOrder" not in rendered_text

    frame = {
        "turn": 12,
        "year": -3900,
        "width": 4,
        "height": 3,
        "terrain": [0] * 12,
        "cities": [{"owner": 1, "x": 1, "y": 1, "pop": 2}],
        "players": [
            {"id": 1, "civ": "Romans", "cities": 1, "score": 50, "dead": False},
            {"id": 2, "civ": "Greeks", "cities": 1, "score": 40, "dead": False},
        ],
        "events": events,
    }
    palette = {0: (70, 120, 70)}
    fonts = (render.load_font(17), render.load_font(13), render.load_font(13))
    image = render.render_frame(frame, palette, 1, True, {1: "Romans", 2: "Greeks"}, fonts, names)

    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "caption-smoke.png"
        image.save(out)
        assert out.exists()
        assert image.width > 0 and image.height > 0

    print("timelapse caption smoke: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
