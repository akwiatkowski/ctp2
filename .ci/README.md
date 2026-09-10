# Local CI

Run project commands through `mise exec --`. CI uses the installed game data
under `CTP2_HOME` (default `~/.ctp2`); licensed assets stay outside the repository.

| Command | Coverage |
|---|---|
| `make ci-tier-a` | Fast C++ build and tests |
| `make ci-tier-b` | Player UI, headless and test builds; unit tests; eight short headless scenarios; fast/installation/CI/harness checks; save replays through rounds 11, 40 and 75 |
| `make ci-nightly` | Sanitized headless/unit build; full C++ unit and integration suite; eight short headless scenarios; 500-round campaign plus deep save/load |
| `.ci/tiers/tier-d.sh` | Standalone UBSan build and full C++ suite |
| `make test-render` | Fifteen pixel/renderer checks, including seeded UI scenes and sprite lifecycles; requires a desktop session |

Each tier returns nonzero on failure and records its result in `.ci/state.json`.
Logs, doctest XML and parsed failures live in `.ci/log/`; `context_path` identifies
the complete log. A build failure, missing/malformed report or failed scenario
cannot be overwritten by a passing unit report. Any red tier sets `STATUS_RED`.
Run `mise exec -- python3 .ci/test_results.py` to verify failure reporting,
including failed builds through all four real shell wrappers.

Tier C uses UBSan on macOS: native ASan currently hangs before `main` on this
macOS 26 host, including a trivial executable. On Linux it uses ASan + UBSan.
Sanitizers halt on the first error. ASan leak detection remains disabled;
a passing run does not establish leak freedom. Existing build directories are
reconfigured to the requested sanitizer, single-player mode (`anet=false`) and
`debugoptimized` before building. Debug symbols remain enabled; optimization
keeps long campaigns practical even when every turn serializes an autosave.

`make ci-start` starts the optional working-tree/commit daemon; `ci-stop`,
`ci-status`, `ci-watch` and `ci-failures` inspect/control it. The daemon runs
Tier A after pending builds and Tier B after HEAD changes. The 300-turn Meson
soak is tagged `marathon`; Tier C runs the stronger 500-turn version separately.
Sanitizer tiers are disabled on commits by default; schedule `mise exec -- make ci-nightly` separately
with cron or launchd. `ENABLE_TIER_C=1` / `ENABLE_TIER_D=1` explicitly restore
post-commit runs, gated on that Tier B invocation succeeding.

Run one CI invocation at a time: tiers share build directories and state. No
nightly service is installed automatically. Hosted coverage lives in
`.github/workflows/ci.yml`: build matrix (macOS arm64 + x64, Ubuntu x64)
compiling all main targets and running the asset-free fast + unit suites,
plus tag-triggered (`v*`) release tarballs. Licensed game data never leaves
maintained machines — that is the whole asset policy. Sanitizer/marathon
runs stay on cron/launchd (see below).

For supervised background work, use the single approvable command prefix
`mise exec -- python3 tools/job.py`:

```sh
mise exec -- python3 tools/job.py start scenarios mise exec -- meson test -C build --suite scenario --no-suite marathon
mise exec -- python3 tools/job.py status scenarios
mise exec -- python3 tools/job.py logs scenarios
mise exec -- python3 tools/job.py stop scenarios
mise exec -- python3 tools/job.py self-test
```

Jobs run from the repository root, with metadata and combined output in
`build/jobs/`. Stop verifies process identities, sends TERM to the job and its
descendants (including separate test sessions), then kills survivors after five
seconds. Only recorded jobs can be stopped. Reusing an inactive name replaces its
log. Status reports whether the process is running; check the log for test results.

## GPU and sprite acceptance

`make test-render` now checks that UI seed 42 reproduces the same terrain and
starting armies in two processes, while seed 43 changes the map. The client
passes its seed/player count to `start_game`; UI seed zero is rejected because
zero means clock-seeded initialization in the legacy engine. Profile settings
still affect generation, so a retained save is the authoritative replay input.

The fog check retains each run under `build/worldmap-fog/<timestamp>/` with its
base save, presented screenshots and renderer state. Replay a failing scene:

```sh
CTP2_HOME="$PWD/build/render-home" mise exec -- python3 ctp2_code/test/worldmap_fog.py --fixture /absolute/path/to/base.sav
```

Desktop and screenshot readback share world presentation and fallback policy.
CPU mirror pixels use origin zero, while GPU window quads include their content
margin. The fog oracle now samples identical screen rectangles in both paths.

`gpu-actor-parity` isolates attack/victory sprite pixels across three engine
scales, mirrored facings and smooth camera transforms. It also checks visible
and fogged resource colours. A separate installation without modern atlases
exercises actual whole-map fallback; disabling modern sprites alone disables GPU
mode and would miss that case. Pixel masks allow one pixel at rasterization edges
and exclude independently animated armies. `gpu-trade-animation` creates a real
funded route and checks appearance, movement and removal with a stationary camera.
These two Python image checks require Pillow.

Generate the sprite review matrix and per-scene GPU coverage table:

```sh
CTP2_HOME="$PWD/build/render-home" mise exec -- python3 tools/visual/render_gallery.py --families sprites --zooms 0 4 5
```

Open the generated `index.html`. `run.json` records options, `base-fixture.json`
retains the scene, `manifest.jsonl` records each capture, and `coverage.md`
reports actual GPU-rasterized and CPU-composited cells during forced rebuilds.
These counts exclude unchanged cached cells and do not measure frame time.
Engine zoom 5 is native size and enables GPU terrain rasterization; zooms 0
and 4 exercise CPU-composited terrain presented through the whole-map texture.
The older `cells_redrawn` diagnostic counts submitted quads, not unique cells.

The matrix samples archer move/attack/idle/victory/work actions at first/middle/
last frames, stored and mirrored facings, three engine zooms, transparency and
fog; it also includes goods, city defenses and an effect. Missing source poses
remain explicit skips, never passes. This is representative coverage, not
all frames of all 463 assets. Both modes normally use modern atlases; use
`--legacy-reference` to compare against legacy SPR software drawing instead.

`debug_sprite_pose` is exposed only by `ctp2_render`. It fixes visual pose and
opacity without advancing gameplay. The runtime lifecycle regression uses
`ctp2`, normal drawing and real selection/visibility/death/cargo operations;
it does not use the fixed-pose hook or recenter after tested transitions.
Full-frame matching percentages are review aids, not acceptance thresholds:
background pixels can overwhelm a missing sprite.

Profile duplicate actor painting on macOS with the system sampler:

```sh
CTP2_HOME="$PWD/build/render-home" mise exec -- python3 tools/visual/profile_sprites.py
```

The tool captures matched 40-actor CPU/GPU scenes, renderer state and ten seconds
of main-thread stacks per mode. Run it without another game or build competing
for CPU time. Sample counts include sleeping stacks and are not FPS or GPU timing.
CPU painting remains required for whole-map sprite fallback and CPU-made overlays.
