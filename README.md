# Call to Power 2 — modern port work

This is my fork of the Call to Power 2 source code. The original Activision
code was released in 2003; the Apolyton community kept it alive after that. I
forked the code from the Apolyton tree to make the game engine build and run on
modern macOS, and to separate the simulation from the old Windows UI so the
game logic can be tested without graphics.

## Goal

The original game only built with Visual Studio and ran on Windows. The
simulation code was mixed with Win32 window calls, DirectDraw rendering, and
sound triggers. I wanted to:

- Build and run the game simulation on macOS (Apple Silicon).
- Make the simulation deterministic and testable from a terminal.
- Add a readable save-file format.
- Clean up the code enough that future work is possible.

## Status

- **macOS** is the platform I develop and test on. The headless binary builds
  and runs.
- **Linux** is targeted by the same Meson build but not tested yet.
- **Windows** is best-effort only. The original VS 2017 solution is still in
  the tree but not maintained as part of this fork.
- No gameplay, balance, AI behaviour, or content changes. This is engine work
  only.

## What I did

- Added a Meson build for the simulation target on macOS/Linux, so the old
  Visual Studio project is no longer needed for headless work.
- Created a headless mode (`ctp2_headless`) that runs the game with no
  graphics, no sound, and no window manager. It can start a game, load/save,
  and autoplay turns from the command line.
- Added a JSON save format that round-trips the full game state. The original
  binary `.c2g` format is still supported. The JSON work also fixed a bug
  where the binary save header was writing uninitialized memory to disk.
- Replaced random-number access throughout the simulation with one `civrand()`
  accessor. Before, many parts of the code read the RNG state directly, which
  broke reproducibility.
- Split the game-state code from the UI/render code using observer interfaces
  in `ctp2_code/gs/core/`. The simulation now talks through bridges like
  `game_observer`, `battle_observer`, and `diplomacy_observer`. Real UI
  implementations live in `ui/` and `gfx/`; the headless binary uses null
  implementations. This means the simulation compiles and runs without linking
  the graphics layer.
- Moved some types that belonged in game state out of graphics code (`TileInfo`,
  `DiplomacyTypes`, audio/colour/pixel types).
- Replaced scattered `printf` logging with `spdlog` behind a thin wrapper.
- Bumped the C++ standard to C++20.
- Removed dead code: COM-style `Ic3CivArchive` / `IRobot` scaffolding, old
  save-format paths, unused build entries.
- Added a Python test harness under `ctp2_code/test/` with smoke tests,
  autoplay runs, and a check that fails CI if new UI calls leak back into
  game-state code.
- Added save/load byte-equality checks to catch uninitialized memory and
  iteration-order bugs.

## How hard it was

The biggest problem was that game logic was not separated from the user
interface. Functions inside `CityData`, `ArmyData`, `Player`, combat,
diplomacy, SLIC scripting, and file I/O were opening windows, drawing text,
playing sounds, and reading the tile map directly. To run the simulation
without the full Windows graphics stack I had to introduce the observer/bridge
layer and move types to their proper places. This touched a large part of the
codebase.

Save/load was also difficult. The binary format was basically raw struct dumps
with uninitialized padding and version-locked fields. Writing a JSON serializer
that covers the whole game state (world, players, cities, armies, SLIC engine
state, and so on) required understanding and mapping many interdependent
structures.

Determinism was tricky too. The game had many direct RNG accesses. Tracking
them all down and routing them through one accessor took time, and the
save/load round-trip tests caught several subtle bugs, including a SLIC segment
hash overflow that corrupted state on reload.

## Build

On macOS / Linux:

```sh
mise exec -- make setup
mise exec -- make build
mise exec -- make test
```

This creates the headless binary in `build/`.

## Repository layout

```
ctp2_code/
  gs/          game-state — simulation, file I/O, SLIC, world
    core/        observer interfaces and type extractions
    fileio/      binary + JSON save/load
    gameobj/     cities, units, armies, players, pools
    slic/        SLIC scripting engine
    world/       map, cells, tiles
  ui/          real UI implementations of the observers
  gfx/         render side
  ai/          AI, now talks to game-state through bridges
  test/        Python harness, smoke tests, C++ unit tests
  3rdparty/    nlohmann/json, spdlog, etc.
```

## Scale

About 480 commits of my work sit on top of the imported upstream history, out
of roughly 2,300 total commits in the repository. The changes are concentrated
in the build system, save/load, core game-state interfaces, and test harness.
