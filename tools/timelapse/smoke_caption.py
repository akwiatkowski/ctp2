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
        "units": {"11": "Legion"},
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

    # "what+where" enrichment: KillUnit resolves the unit id, and location
    # args resolve to a known city name instead of raw coords.
    cities = [{"owner": 1, "x": 10, "y": 10, "pop": 4, "name": "Rome"}]
    ev2 = [
        {"event": "KillUnit", "player": 0,
         "args": [{"kind": "unit", "value": 11}]},
        {"event": "CreateBuilding", "player": 1,
         "args": [{"kind": "int", "value": 3},
                  {"kind": "location", "x": 10, "y": 10}]},
        {"event": "ImprovementComplete", "player": 1,
         "args": [{"kind": "int", "value": 1},
                  {"kind": "location", "x": 12, "y": 11}]},
    ]
    text2 = "\n".join(t for _, t in render.frame_captions(ev2, names, cities))
    assert "Legion" in text2
    assert "Granary in Rome" in text2
    assert "near Rome" in text2
    # Far-away unknown location falls back to coords
    ev3 = [{"event": "CreateBuilding", "player": 1,
            "args": [{"kind": "int", "value": 3},
                     {"kind": "location", "x": 99, "y": 99}]}]
    text3 = "\n".join(t for _, t in render.frame_captions(ev3, names, cities))
    assert "at 99,99" in text3

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
