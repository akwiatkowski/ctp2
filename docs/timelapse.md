# CTP2 Timelapse

The timelapse pipeline has two phases:

1. Record one autoplay game into `run.jsonl` plus optional raw map frames.
2. Re-render that recording into PNG frames and `timelapse.mp4` as many times as needed.

Use `mise exec --` indirectly through the Makefile targets.

## Common Commands

Record 150 turns and render the video:

```sh
mise exec -- make timelapse
```

Record a longer run into a custom output directory:

```sh
mise exec -- make timelapse TURNS=300 TL_DIR=/tmp/ctp2-timelapse-300
```

Render from the last recording again without replaying the game:

```sh
mise exec -- make timelapse-render
```

Render from a custom recording directory:

```sh
mise exec -- make timelapse-render TL_DIR=/tmp/ctp2-timelapse-300
```

Smoke-test Chronicle caption rendering without running the game:

```sh
mise exec -- make timelapse-caption-smoke
```

## Useful Options

- `TURNS=300`: number of autoplay turns to record.
- `TL_DIR=/tmp/ctp2-timelapse`: output directory; contains `run.jsonl`, `raw/`, and `frames/`.
- `TL_PLAYER=-1`: unfogged whole map. Set `TL_PLAYER=1` or another player id for a fogged empire view.
- `TL_REALART=1`: use real isometric engine-rendered frames. Use `TL_REALART=0` for the stylized fallback renderer.
- `TL_ZOOM=5`: engine render zoom, `0..5`; `5` keeps native tile detail.
- `TL_FPS=12`: output video framerate.
- `TL_TILE=9`: tile size for the stylized fallback renderer.
- `TL_MAPW=0`: final frame width; `0` means no downscale.

The final video is written to:

```text
$TL_DIR/frames/timelapse.mp4
```

## Notes

- Recording is slower because it runs the game and queries per-turn state.
- Rendering is cheap and can be repeated while tuning captions, frame size, FPS, or visual style.
- Chronicle captions use `query_names` metadata when present, so events can show technology, wonder, building, and improvement names instead of raw IDs.
