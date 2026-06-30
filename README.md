# Call to Power 2 — fork

Fork of the [Apolyton CtP2 Source Code project](https://apolyton.net/forum/other-games/call-to-power-2/ctp2-source-code-project),
which itself descends from the original Activision source released in 2003.
Code was imported from <https://ctp2.darkdust.net/anonsvn/>.

This README describes what has actually been changed in this fork, not what is
planned. It is written for other players and developers who want to know
whether the changes here are useful to them.

## Platform support

- **macOS** is the primary target. Everything here is developed and tested on
  macOS (Apple Silicon).
- **Linux** is an explicit goal via the same Meson build. Not actively tested
  yet, but nothing in the refactors is macOS-specific — it should work or be
  close to working.
- **Windows** is *not guaranteed*. The upstream VS 2017 solution is still in
  the tree and may still build, but it is no longer the focus and may have
  bit-rotted. If you want a known-good Windows build, use the upstream
  Apolyton tree instead of this fork.

## TL;DR

- macOS-first fork. Linux targeted, Windows best-effort.
- There is now a **Meson build**, a **headless mode**
  (no graphics, no sound, runs from a terminal), a **JSON save format**, and a
  large amount of refactoring that pulls the game logic away from the UI/render
  code so it can be tested in isolation.
- C++ standard bumped to **C++20**.
- Logging goes through **spdlog**.
- About **480 commits** worth of work sits on top of upstream.

Nothing about gameplay rules, balance, scenarios or mods has changed. This is
all engine / plumbing work.

## How to build

### macOS / Linux (Meson)

```sh
mise exec -- make setup
mise exec -- make build
mise exec -- make test
```

This configures `build/`, compiles the project, and runs the fast pre-commit
test tier. The headless binary runs the simulation without graphics, sound, or
window manager. It is useful for AI work, save-file inspection, determinism
testing, CI, and poking at the game from a shell.

For sanitizer smoke testing, use:

```sh
mise exec -- make setup-ubsan
mise exec -- make ubsan-smoke
```

There is also a `sanitized-smoke` target for ASan+UBSan, but ASan currently
hangs before `main` on the tested macOS/Apple clang setup. UBSan is the working
sanitizer tier for now.

macOS is what this is tested on. Linux should work via the same setup but
hasn't been exercised yet — patches welcome.

### Windows (best-effort)

The original `ctp2_code/ctp/civctp.sln` Visual Studio 2017 solution is still
in the tree. It may still build with the C++ workload + Windows 10 SDK, but
it is no longer maintained as part of this fork and may break with new
refactors. If you specifically want Windows, you probably want the upstream
Apolyton tree.

## Headless mode

`ctp2_headless` can:

- Start a new game from a seed (`--seed 0` gives a reproducible map and AI run).
- Load a save file (binary or JSON, auto-detected).
- Save back out, in either format.
- Run autoplay for N turns and exit.
- Convert binary saves to JSON and back via the same CLI flags.

There is a Python test harness around it under `ctp2_code/test/` (autoplay
runs, smoke tests, integration suite). Tests track per-test timing and there
is a "UI command-surface ratchet" that fails CI if new direct UI calls leak
back into the game-state code.

## JSON save format

The original `.c2g` binary save format is preserved and is still the default
on Windows. In addition, there is now a full **JSON save format** that
round-trips the entire game state:

- World, cells, tiles, unseen tiles, quadtrees
- All player data: civ, diplomacy, agreements, regard, threats, gaia,
  happiness, score, feats, build queues, top-ten, end-game state
- Cities (CityData), tile improvements, installations, trade routes, goody huts
- Armies, units, orders, paths, pools
- The SLIC engine state: symbol tables, segments, structs, arrays, named
  symbols, records, constants, contexts, objects, messages, buttons, eyepoints

The headless binary writes JSON by default (`SaveGame`) and load auto-detects
which format is on disk. There is a binary→JSON converter mode for inspecting
or diffing existing `.c2g` saves as plain text.

Why JSON: the binary format is opaque, version-locked, and full of raw struct
dumps with uninitialised padding. The JSON path also fixed a latent bug where
the binary save header was leaking uninitialised stack and heap bytes to disk.

## Determinism

- Every consumer of random numbers in the simulation has been migrated onto a
  single `civrand()` accessor. There used to be a dozen call sites poking at
  RNG state directly; now there is one.
- A `ScopedRand` test fixture lets tests pin the RNG for the duration of a
  block.
- A save-file byte-equality scaffold checks that
  `save → load → save` produces the same bytes, which catches uninit memory
  and order-of-iteration bugs.
- A `SlicSegmentHash` overflow that corrupted re-loaded SLIC state was fixed
  along the way.

## Decoupling game state from UI / render

The biggest single refactor. In the original code the game logic
(`CityData`, `ArmyData`, `Player`, combat, diplomacy, SLIC, file I/O,
endgame…) called directly into the UI: it opened windows, drew text, played
sounds, asked the tiled map for vision, queried the selected unit, told the
HUD to refresh, and so on. That made the simulation impossible to run
without a full Win32 + DirectDraw stack standing behind it.

This fork introduces a layer of **observer / bridge interfaces** in
`ctp2_code/gs/core/`. The game state code now talks to these:

- `player_view` — selected unit / city / tile, scenario editor bindings,
  edit-queue, save/load selection.
- `game_observer` — endgame window, modal messages, begin-turn messages,
  HUD queries.
- `battle_observer` — combat events (replaces direct `Battle*` UI calls).
- `diplomacy_observer` — diplomatic events / markers.
- `text_observer` — replaces direct `primitives_DrawText` calls from the
  simulation.
- `progress_observer` — load/save progress.
- `tiledmap_observer` — vision, tile queries, lifecycle.
- `gfx_options_observer` — graphics options the AI used to read directly.
- `colorset_observer` — colour lookups.
- `audio_observer` — sound triggers.
- `render_observer` — explicit redraw requests.

Real UI implementations of these live in `ctp2_code/ui/` and `gfx/`. The
headless binary plugs in null implementations. Everything in `gs/` now
compiles and runs without `gfx/` or `ui/` linked in.

In addition:

- `TileInfo` moved from `gfx/tilesys/` to `gs/world/` — it was never really
  a graphics object.
- `DiplomacyTypes` moved from `ai/diplomacy/` to `gs/diplomacy/`.
- Sound/music enums and `gamesounds_*` IDs extracted to
  `gs/core/audio_types.h`.
- Pixel typedefs extracted to `gs/core/pixel_types.h`, colour enums to
  `gs/core/color_types.h`.
- `SpriteStatePtr` and `UnitActorPtr` forward declared in
  `gs/core/sprite_state_fwd.h` so game-state headers don't drag in render
  types.
- A multi-phase split of `UnitActor` is in progress: the game-state half
  (`UnitState`) has been scaffolded out from the render-side actor;
  obsolete caches on `UnitActor` (`m_isFortified`, `m_isFortifying`,
  `HasCityWalls`, `HasForceField`, `m_savePos`) have been deleted; a
  shadow UI actor registry now owns spawn events.

## Dead code removed

- The entire COM-style `Ic3CivArchive` / `IRobot` / `BSet` /
  `COM_INTERFACE` scaffolding that nothing was using.
- Leftover COM scaffolding inside `CivArchive` itself.
- `USE_FORMAT_67` and other dead save-format paths.
- 19 dead `gs/outcom/` entries from the Meson build.

## Logging

`printf` / `DPRINTF` chatter has been routed through **spdlog** behind a thin
`civlog::Get` / `civlog::Init` shim. Log level, sinks and format are
controlled in one place instead of being scattered through the source.

## Build / language

- C++ standard bumped to **C++20**.
- `nlohmann/json` v3.11.3 vendored as a single header under `3rdparty/`.
- spdlog vendored similarly.
- Meson build for the headless target, alongside the original VS solution.

## What has *not* changed

- No gameplay, balance, rules, scenarios, mods, AI behaviour, or unit/civ
  data changes.
- No new graphics engine. The upstream goal of replacing DirectDraw with a
  D3D11 scene-based renderer (see [RolandTaverner's d3dui
  branch](https://github.com/RolandTaverner/ctp2/tree/d3dui)) is not part
  of this fork.
- No native macOS/Linux UI yet — the full graphical client only exists on
  Windows (and even there, only best-effort, see above). On macOS/Linux you
  get the headless binary.

## Repository layout (interesting parts)

```
ctp2_code/
  gs/            game-state — simulation, file I/O, SLIC, world
    core/          observer interfaces, type extractions
    fileio/        binary + JSON save/load
    gameobj/       cities, units, armies, players, pools
    slic/          SLIC scripting engine
    world/         map, cells, tiles
    diplomacy/     diplomatic types
    utility/       civrand() and friends
  ui/            real UI implementations of the observers
  gfx/           render side
  ai/            AI, now talks to game-state through bridges
  test/          Python harness, smoke tests, C++ unit tests
  3rdparty/      nlohmann/json, spdlog, etc.
```

## Original Apolyton notes

> Written with [StackEdit](https://stackedit.io/).
