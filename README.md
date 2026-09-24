# Call to Power 2 — native macOS & Linux port

Call to Power 2 is a turn-based 4X strategy game from 1999. Activision released
its source in 2003 — written for Visual Studio 6, welded to DirectX, and mute,
because the sound library could not be included. The Apolyton community has kept
it compiling on Windows and Linux ever since, but macOS was never solved: their
["Building on OSX"](https://github.com/civctp2/civctp2/issues/350) issue has been
open since 2020. This fork runs the real, playable game natively on Apple
Silicon — SDL3, a GPU render path, working audio, inertial trackpad panning and
pinch zoom. No Wine, no VM, no Windows. Linux x86-64 builds from the same tree.
You bring your own copy of the game data.

- **Native on three targets.** macOS arm64, macOS x86-64 and Linux x86-64 all
  build on every push in CI; tagged commits publish binaries.
- **Modern feel.** SDL3 with GPU layer compositing: inertial trackpad pan,
  pinch zoom, hardware cursor, 32-bit color. `CTP2_GPU_LAYERS=0` drops back to
  the legacy CPU present if you want to compare.
- **Sound works.** Activision could not ship the Miles sound library with the
  source, so the released code was mute. Audio here runs on SDL3_mixer.
- **Bring your own data.** A complete game installation was never part of the
  source release. One script copies *your* copy into `~/.ctp2` and converts its
  sprites into 463 modern texture atlases — on your machine, never committed.
- **Far fewer crashes.** 963 catalogued static-analysis findings; all 54
  CRITICAL ones verified in code and fixed; the HIGH crash classes (bad shifts,
  division by zero, missing bounds checks, null derefs) burned down. The UBSan
  tier runs 643 unit cases with zero findings.
- **Headless and deterministic.** `ctp2_headless` runs the whole simulation with
  no window, no GPU and no sound. Every RNG read goes through one seeded
  accessor, so a seed replays identically. Saves round-trip through JSON.
- **A 4X game an LLM can actually play.** The bundled gateway serves the running
  engine over HTTP and exposes 51 MCP tools (`start_game`, `move_army`,
  `set_production`, `declare_war`, …).
- **Tests that look at pixels.** 290 fast + 643 unit C++ cases, 39 Meson tests,
  and 15 renderer oracles that compare actual presented frames against the CPU
  reference.

Five rounds of a seeded three-player game, no window, straight to CSV:

```console
$ ./build/ctp2_headless --new-game --players 3 --seed 42 --turns 5 --export-metrics -
# PLAYERS
player_idx,leader_name,is_dead,total_score,gold,num_cities
0,Attila,no,0,500,0
1,Julius Caesar,no,2090,100,1
2,Tomoe,no,1900,523,2

# CITIES
player_idx,city_name,pos_x,pos_y,population,visible_owner,explored_owner
1,Rome,34,11,1,yes,yes
2,Nara,18,39,1,yes,yes
2,Nagaoka,23,37,1,yes,yes
```

Same seed, same numbers, every time.

---

## Contents

- [Getting started](#getting-started)
- [Playing](#playing)
- [Headless mode](#headless-mode)
- [The gateway: HTTP and MCP](#the-gateway-http-and-mcp)
- [Tests](#tests)
- [What is different from upstream](#what-is-different-from-upstream)
- [Status and limitations](#status-and-limitations)
- [Repository layout](#repository-layout)
- [More documentation](#more-documentation)
- [Credits and license](#credits-and-license)

---

## Getting started

> **You need your own copy of Call to Power 2.** A complete installation's
> graphics, sound and data were never part of the source release. This tree
> carries the `ctp2_data/` directory inherited from the Apolyton project — text
> databases, UI layouts and some assets — but that is not a playable data set on
> its own. Any legal installation works: retail CD, GOG, Steam. You just need to
> point at its `ctp2_data` directory.

### 1. Install build dependencies

**macOS** (Homebrew):

```sh
brew bundle              # or: make deps
```

**Linux** (Ubuntu 24.04 and similar):

```sh
sudo apt-get update
sudo apt-get install -y meson ninja-build pkg-config libtiff-dev zlib1g-dev \
    byacc flex cmake libasound2-dev libpulse-dev
```

Ubuntu 24.04 predates SDL3 packaging, so build SDL3 and SDL3_mixer from their
releases (the exact versions CI uses are in
[`.github/workflows/ci.yml`](.github/workflows/ci.yml)):

```sh
curl -L https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz | tar -xz -C /tmp
cmake -S /tmp/SDL3-3.4.16 -B /tmp/SDL3-3.4.16/build -DCMAKE_BUILD_TYPE=Release
sudo cmake --build /tmp/SDL3-3.4.16/build --target install -- -j"$(nproc)"

curl -L https://github.com/libsdl-org/SDL_mixer/releases/download/release-3.2.4/SDL3_mixer-3.2.4.tar.gz | tar -xz -C /tmp
cmake -S /tmp/SDL3_mixer-3.2.4 -B /tmp/SDL3_mixer-3.2.4/build -DCMAKE_BUILD_TYPE=Release
sudo cmake --build /tmp/SDL3_mixer-3.2.4/build --target install -- -j"$(nproc)"
sudo ldconfig
```

### 2. Build

```sh
git clone https://github.com/akwiatkowski/ctp2.git
cd ctp2
make setup      # meson setup build ctp2_code --buildtype=debug
make build      # meson compile -C build
```

This produces `build/ctp2` (the game), `build/ctp2_headless` (the simulation
with no UI) and the test binaries. That is a debug build with every check on —
fine to play, but `make release` builds an optimized, thin-LTO binary that runs
turns about 5x faster in wall clock and produces bit-identical results.

> A few `make` targets shell out through `mise exec --` (the maintainer's
> toolchain manager). If you do not use [mise](https://mise.jdx.dev), either
> install it or run the underlying `meson` / `python3` command directly — the
> Makefile shows each one.

### 3. Install your game data

```sh
python3 tools/assets/install_ctp2_home.py /path/to/your/ctp2_data
```

This copies the original data verbatim into `~/.ctp2/original_data/` and
generates modern sprite atlases into `~/.ctp2/assets/`. Both stay on your
machine; the atlases are derived from data you own and are rebuildable at any
time. Set `CTP2_HOME` to install somewhere else, or pass
`--skip-modern-assets` to copy the original data only.

### 4. Play

```sh
make run                            # 1920x1080
./build/ctp2 --resolution 2560x1440 # or pick your own
```

---

## Playing

Runtime switches, all optional:

| Variable | Effect |
|---|---|
| `CTP2_HOME` | Data + save root (default `~/.ctp2`) |
| `CTP2_GPU_LAYERS=0` | Legacy CPU present instead of GPU layer compositing |
| `CTP2_GPU_WORLDMAP=0` | Legacy whole-map path |
| `CTP2_GPU_RASTER=0` | CPU rasterization for roads, borders, grid, improvements |
| `CTP2_MODERN_SPRITES=0` | Force the original `.SPR` sprites over the atlases |
| `CIVLOG_LEVEL` | Log verbosity (`trace`…`error`) |

### Recorded human playtests

```sh
mise exec -- make play-record
```

No session name or seed is required. The launcher generates both, opens the
normal visible game, and writes to `build/playtests/<timestamp>-<commit>-<id>/`;
`build/playtests/latest` points at the active/latest run. It copies the current
profile instead of modifying it and records the commit, dirty state, seed,
profile hash, asset fingerprint, native input, resolved LDL targets, game log,
passive last frame, per-turn JSON saves and state snapshots.

When a problem appears, report it immediately. While the game is responsive the
assistant can run:

```sh
mise exec -- make play-mark NOTE="sprites disappeared after clicking the map"
```

This addresses the active session automatically and stores a checkpoint, frame,
queries and note under its `bugs/` directory. If the game is frozen, the
recorder retains the last flushed input/frame and captures a macOS process
sample without requiring the game to answer. Keep the recorder terminal open
until the game exits.

After a fix, continue from the newest checkpoint (a marked bug checkpoint takes
priority over its earlier turn checkpoint) without naming a session:

```sh
mise exec -- make play-resume
```

This starts a new recorded session linked to the previous checkpoint and reuses
the copied profile; the original evidence remains immutable.

The bug bundle is evidence, not automatically a good assertion. The repair
workflow is: reproduce from the nearest turn checkpoint, retain the shortest
reliable input suffix, add an observable oracle for the reported symptom, prove
it fails before the fix, then add that replay as a permanent offscreen full-UI
test. A general replay/minimizer is intentionally deferred until real recorded
bugs establish which UI context must be restored.

If the game crashes, [`run_game.sh`](run_game.sh) runs the sanitized build and
captures stdout plus a backtrace into `test/crashes/` — attach that to a bug
report. Note that AddressSanitizer binaries currently hang before `main` on
macOS 26 (a sanitizer-runtime bug that hits even a trivial program), so
`make ubsan-smoke` is the working sanitizer tier there; ASan is fine on Linux.

`make timelapse` records an AI-vs-AI game and renders the empires spreading
across the map as a video. See [`docs/timelapse.md`](docs/timelapse.md).

---

## Headless mode

`ctp2_headless` is the full simulation without graphics, sound or a window. It
is what makes the engine testable, scriptable and reproducible.

```sh
./build/ctp2_headless --new-game --players 3 --seed 42 --turns 100 \
    --save-game /tmp/turn100.json --export-metrics /tmp/metrics.csv
./build/ctp2_headless --load-game /tmp/turn100.json --turns 10
```

| Flag | Meaning |
|---|---|
| `--new-game` | Generate a world and start |
| `--players N` | Number of players (default 3) |
| `--seed N` | Pin the RNG seed |
| `--turns N` | Run N rounds, then exit |
| `--save-game PATH` | Save the game as JSON and exit |
| `--load-game PATH` | Load a JSON save instead of generating a world |
| `--export-metrics PATH` | Per-player and per-city CSV (`-` for stdout) |
| `--serve` | Listen on a command socket instead of autoplaying |

The JSON save format round-trips the complete game state — world, players,
cities, armies, scheduler graph, SLIC engine — and is diffable, which is how
several state-corruption bugs were found in the first place. It is now the only
save format: the legacy binary `.c2g` path has been removed, so saves made by
the original game do not load here.

---

## The gateway: HTTP and MCP

[`gateway/`](gateway/README.md) is a small Crystal server that fronts a running
`--serve` game and gives it four faces on one port: a curl-shaped JSON API, a
health endpoint, omniscient admin pages, and an MCP server.

Building it needs [Crystal](https://crystal-lang.org):

```sh
cd gateway && shards build
./bin/ctp2-gateway --spawn --spawn-args "--players 3 --seed 42"

curl localhost:8666/healthz
curl -X POST localhost:8666/api/cmd -d '{"cmd":"start_game"}'
curl localhost:8666/api/cities
```

To let Claude play:

```sh
claude mcp add --transport http ctp2 http://localhost:8666/mcp
```

51 tools cover founding cities, moving armies, production, research, taxes,
governors, trade, diplomacy and war, plus save/load checkpointing so a model can
branch an experiment and come back. Game-level refusals (`no_settler_found`,
`settle_rejected`) come back as tool errors with detail, so the model can adapt
instead of guessing. Notes from real sessions live in
[`docs/play-sessions/`](docs/play-sessions/).

---

## Tests

```sh
mise exec -- make test  # ratchets + fast + unit + isolated menu interactions
mise exec -- make test-ui-integration  # full game, synthetic input, no visible window/audio
make test-full      # the long suite: integration, smoke, scenarios (~20-30 min)
make test-render    # 15 renderer pixel oracles (needs a desktop session)
make ubsan-smoke    # UndefinedBehaviorSanitizer tier
```

`ui-menu` runs the real menu layouts, mouse hit-testing, queued actions, and
dirty-window compositing on SDL's dummy/software backend. It checks New Game,
Back/reopen, difficulty selection/persistence, and exactly one launch request
with the selected settings. No desktop, audio, session scripts, world generation,
or AI is started; installed game assets are still required. Run it alone with
`mise exec -- meson test -C build ui-menu --print-errorlogs`. Passive software
frame captures are saved under `build/ui-menu/`; they do not force a redraw and
do not claim to verify native GPU presentation or the game map.

`ui-offscreen` is the full-game integration tier. It boots the real application
with a fixed seed, clicks New Game and Launch, hovers terrain, and clicks distant
minimap positions before returning to explored terrain. SDL dummy video/audio
and the software renderer keep it windowless and silent. It checks camera
movement, responsiveness, and terrain pixels outside the HUD; no callbacks,
debug reveal, or forced-repaint screenshots substitute for player input.
Artifacts (frames, state trace, game log) are retained under
`build/ui-offscreen-*/`. The focused cursor test in `ui-menu` additionally uses
guarded overlay memory to detect offscreen writes even when they do not crash.

`make test-ui-integration` runs the full `ui-integration` suite:
`ui-offscreen`, `ui-sprite-clicks`, `ui-edge-scroll`, `ui-ten-turns`,
`ui-next-unit`, `ui-resume-controls`, `ui-resume-map`, `ui-native-input`,
`ui-move-visibility`, and `play-record-smoke`. The sprite scenario checks
animated actor pixels after map/minimap clicks. The edge-scroll scenario
requires camera progress and updates the minimap view rectangle.
`ui-next-unit` is the first regression promoted from a recorded human session:
it loads the captured turn-1 checkpoint and replays the exact Next Unit control
twice, requiring prompt responses and a subsequent normal frame.
`ui-resume-controls` queues native SDL motion/button-down/button-up events after
loading that checkpoint, proves the button-down reaches the real TurnButton,
and requires the normal Director pipeline to advance the round.
`ui-resume-map` reproduces accumulated camera pan after loading a checkpoint,
clicks the real radar map, and requires the camera offset to reset before the
next passive frame.
`ui-native-input` queues a 200-motion backlog before SDL button transitions and
requires the press to reach the control within half a second. `ui-move-visibility`
moves the recorded settler into fog and requires newly visible terrain/resource
pixels in the next passive frame without camera movement.

`ui-ten-turns` completes exactly ten rounds using the real Next Turn button
(not the synchronous automation round runner). It tracks stable starting-unit
IDs, founds Rome, moves the original second settler to found Pompeii, enables
the Growth governor, and checks population growth plus autonomous production
continuing afterward. A private Beginner profile supplies two starting settlers;
ordinary farmer assignments prioritize food until population grows. No units,
food, population, production, or turns are granted by debug commands. Per-round
state and passive frames are kept under `build/ui-ten-turns-*/`; failure reports
include a process sample on macOS. Query city/army/unit `id` fields remain stable
when list indices change.

For additional scripts, use `Ctp2Client` in UI mode with `SDL_VIDEO_DRIVER=dummy`,
`SDL_RENDER_DRIVER=software`, `SDL_AUDIO_DRIVER=dummy`, and
`CTP2_CAPTURE_FRAMES=1`. The smoke-mode commands are:

| Command | Behavior |
|---|---|
| `ui_control_bounds <LDL path>` | Screen rectangle, effective visibility/enabled state, and pressed (`down`) state |
| `ui_pointer <x> <y> <down>` | Direct AUI pointer dispatch; `down` is 0 or 1 |
| `ui_native_pointer <x> <y> <down>` | Queue SDL motion and button-down/up events; use 0 → 1 → 0 to click |
| `ui_prepare_game <seed> <players>` | Pin setup before clicking Launch |
| `screenshot_frame <path>` | Save the last normal pre-present frame with its sequence number |

Frame capture is opt-in and passive: it never invalidates, redraws, or presents
on request. These checks cover real SDL rendering in memory, not Metal-specific
behavior or native macOS input translation.

The renderer tests are the unusual ones: they drive the real game, read back the
presented frame, and compare it pixel-by-pixel against the CPU reference path.
That is how a GPU world layer that had silently been empty for an entire phase
was eventually caught.

A local tiered CI ([`.ci/README.md`](.ci/README.md)) runs the longer work —
scenario suites, a 500-round campaign with save/load every turn, sanitizer
builds — on machines that have the licensed game data. Hosted GitHub Actions
builds all three platforms and runs the asset-free tiers, because the data must
never leave your machine.

`make test` also enforces a **modernization ratchet**: counts of raw
`new`/`delete`, unsafe string APIs, C allocation and type-erased casts may fall,
never rise.

---

## What is different from upstream

Compared to the Apolyton `civctp2` tree this descends from:

- **Meson + Ninja** instead of the Visual Studio solution and autotools.
- **SDL3 only.** The DirectX backend and the SDL2 path are gone.
- **All SDL input on the main thread.** The original `MouseThread` called SDL
  concurrently with the main thread — fatal on macOS, racy everywhere else.
- **GPU render path** for the world map, fog, overlays and camera, with the CPU
  path kept as the pixel-level reference the tests compare against.
- **Simulation separated from the UI** through observer interfaces in
  `gs/core/` (`game_observer`, `battle_observer`, `diplomacy_observer`, …), so
  game state compiles and runs without linking graphics at all.
- **JSON saves**, one RNG accessor, `spdlog` instead of scattered `printf`,
  C++20, and about 4,600 lines of dead `#if 0` and MSVC 6 scaffolding deleted.
- **Modern asset pipeline** — `.SPR` sprites decoded and repacked into texture
  atlases, locally, from data you own.

[`COLLABORATION.md`](COLLABORATION.md) goes through what each tree could take
from the other.

---

## Status and limitations

- **macOS** (Apple Silicon and Intel) is the primary platform — developed and
  played there daily.
- **Linux x86-64** builds in CI on every push. Less play-tested.
- **Windows** is not maintained here. The original VS solution is still in the
  tree but nobody keeps it working.
- **Single-player only.** The legacy Anet multiplayer stack is preserved behind
  `-Danet=true` but is not built by default.
- **No movies.** The intro and wonder videos used DirectShow and do not play on
  SDL builds; the code is kept for a future SDL-based repair.
- **Original saves do not load.** The binary `.c2g` reader was replaced by the
  JSON format; games saved by the 1999 release cannot be resumed here.
- **No gameplay changes.** No balance, AI-behaviour or content edits. This is
  engine work — the game plays the way it played in 1999.

---

## Repository layout

```
ctp2_code/
  ctp/         app entry, main loop
  gs/          game state — the simulation core
    core/        observer interfaces (the UI/simulation seam)
    gameobj/     Unit, Army, CityData, Player, pools
    fileio/      JSON save/load, path resolution
    slic/        SLIC scripting language (lexer, parser, VM)
    database/    record DBs generated from ctp2_data text schemas
    world/       map, cells, terrain
  gfx/         sprites, tile renderer, GPU layers
  ui/          aui_* widgets, interface windows
  ai/          goal/plan/mission AI
  sound/       SDL3_mixer audio
  test/        Python harness, scenarios, pixel oracles, C++ doctest suites
gateway/       Crystal HTTP + MCP server
tools/         asset converter, timelapse, modernization ratchet, local CI
docs/          design and reference documents
```

## More documentation

| Document | What is in it |
|---|---|
| [`AGENT.md`](AGENT.md) | Onboarding: build, layout, the systemic 1999-era pitfalls |
| [`REFACTORING_PLAN.md`](REFACTORING_PLAN.md) | What is done, what remains, in what order |
| [`ADR_DECISIONS.md`](ADR_DECISIONS.md) | Why the non-obvious design choices were made |
| [`BUG_HUNT_REPORT.md`](BUG_HUNT_REPORT.md) | The 963-finding static-analysis catalogue and its resolutions |
| [`MEMORY_SAFETY_STRATEGY.md`](MEMORY_SAFETY_STRATEGY.md) | Sanitizer tiers and the RAII conversion plan |
| [`docs/modern-assets.md`](docs/modern-assets.md) | Asset pipeline design and the licensing constraints |
| [`gateway/README.md`](gateway/README.md) | HTTP routes, MCP tools, status-code contract |
| [`.ci/README.md`](.ci/README.md) | Local CI tiers |

---

## Credits and license

Call to Power II is copyright © 2001 Activision, Inc. Activision released the
source code in 2003 under the terms in
[`EULA - Source Code for CTP2.rtf`](EULA%20-%20Source%20Code%20for%20CTP2.rtf)
and `Activision CTP2 Source Code_Readme.txt`. The
[Apolyton CtP2 Source Code Project](https://github.com/civctp2/civctp2) kept it
alive for two decades; this fork started from
[RolandTaverner/ctp2](https://github.com/RolandTaverner/ctp2).

A complete set of game data — graphics, sound, text, scenarios — is **not**
covered by that release. Supply it from your own legal copy. The generated
sprite atlases are derived works of that data: they live in `~/.ctp2` on your
machine and must never be committed or redistributed. See
[`docs/modern-assets.md`](docs/modern-assets.md) for the full reasoning.
