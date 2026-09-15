# AGENT.md — CTP2 onboarding for LLM agents

This is a fork of the Apolyton CtP2 source (Activision 1999 codebase, ~300K LOC C++).
Read this before touching anything. Cross-references to deeper docs:
`BUG_HUNT_REPORT.md` (963 known bugs catalogued), `MEMORY_SAFETY_STRATEGY.md`
(sanitizer/refactor plan), `COLLABORATION.md` (diffs vs upstream civctp2).

---

## 1. What this fork is

- Native **macOS (Apple Silicon, ARM64) SDL2 build** of CTP2. Original target was
  Win32 + DirectX. Linux SDL build also works.
- Build system migrated to **Meson + Ninja** (upstream still uses MSVC `.sln` /
  autotools).
- All SDL input on the **main thread** — the original `MouseThread` is removed.
  Do not reintroduce thread-based event polling; SDL on macOS will crash.
- Headless **smoke-test harness** (Unix-socket command server) — see §6.

The DirectDraw → D3D11 rewrite mentioned in `README.md` is the **upstream**
RolandTaverner branch goal, not this fork. Here we keep the SDL software
renderer and stabilize the engine.

---

## 2. Build

```bash
# Standard build (use this for normal play / quick rebuilds)
mise exec -- meson setup build
mise exec -- ninja -C build ctp2

# Sanitized build (use this whenever debugging crashes or fixing bugs)
mise exec -- meson setup build-sanitized --buildtype=debug -Db_sanitize=address,undefined
mise exec -- ninja -C build-sanitized ctp2
```

The sanitized binary is ~3× slower but turns silent corruption into actionable
stack traces. ASan/UBSan suppressions live in `ubsan-suppressions.txt`.

**Always rebuild before trusting sanitizer output.** `autoplay_run.sh` warns
when source is newer than the binary — heed it.

Game data is in `ctp2_data/` (Activision-shipped, do not move). User profile
and saves default to `save/`. Run with `./run_game.sh` to auto-capture
stdout+stderr to `test/crashes/`.

---

## 3. Source layout (only the parts that matter)

```
ctp2_code/
├── ctp/                  app entry, civapp.cpp, civ3_main.cpp
├── gs/                   "game state" — the simulation core
│   ├── gameobj/          Unit, Army, CityData, Player, Pool classes (THE core)
│   ├── fileio/           GameFile.cpp (savegame I/O), CivPaths
│   ├── slic/             SLIC scripting language (lexer/parser + VM)
│   ├── database/         record DBs (Unit, Building, Wonder, Advance…)
│   ├── dbgen/            code generator: .txt schemas → C++ record classes
│   ├── newdb/            newer DB generator (used for newer record types)
│   ├── world/            map, Cell, terrain
│   └── events/           game event queue
├── gfx/
│   ├── spritesys/        sprite + RLE pixel decoders (crash-prone, see §5)
│   ├── tilesys/          tiledmap.cpp tile renderer
│   └── layers/           render layers
├── ai/                   Goal/Plan/Mission AI (settlemap, ctpai.cpp)
├── ui/                   aui_* widgets, interface windows
├── sound/                civsound.cpp
├── os/linux/             Linux/macOS portability shims
├── libs/                 vendored third-party (SDL etc.)
└── test/                 smoke test (server + python harness), cpp/ unit tests
ctp2_data/                read-only game assets per language
save/games/<profile>/     binary save files, magic header "CTP00xx"
```

Generated code: `dbgen` reads schema `.txt` files at build time and emits
`*Record.cpp/h` into the build dir (`build/AdvanceRecord.cpp` etc.).
**Don't edit generated files** — change the schema or the generator.

---

## 4. Data, DBs, SLIC

- **Record DBs** (`UnitRecord`, `BuildingRecord`, …) are loaded from
  `ctp2_data/default/gamedata/*.txt` via parsers produced by `dbgen` / `newdb`.
  Index assignments are **order-dependent**; reordering rows in `.txt` files
  changes integer IDs and breaks old saves.
- **SLIC** (`gs/slic/`) is the in-game scripting language used for events,
  triggers, and UI text. Flex/byacc grammar (`slic.l`, `slic.y`). Errors land
  in `slicdbg.txt` and `usercritmsgs.txt` at runtime.
- **String tables**: `ctp2_data/<lang>/gamedata/ldl_str.txt`. Missing keys
  produce visible `STRING_NOT_FOUND` placeholders.

---

## 5. Known systemic issues (read before "fixing")

This codebase is from 1999. Patterns to expect:

| Pattern | Where | Notes |
|---|---|---|
| Enums with negative sentinel values used as array indices | `UNITACTION`, attack types, etc. | UBSan catches; wrap accessors in bounds checks |
| `(uint64)1 << index` where index ≥ 64 | `GaiaController.cpp` and similar | UB; guard or use bitset |
| `(uint32)&ptr` for pointer arithmetic | `CityData.cpp:760, 1149` | Truncates on 64-bit, corrupts memory. Use `size_t` |
| Manual `new`/`delete`, no smart pointers | Everywhere | Don't introduce smart pointers piecemeal — touch one subsystem at a time |
| `strlen(x) - N` in unsigned context | `Goal.cpp:3255` etc. | Underflows to huge value → stack overflow |
| Format string `%d` for `int64_t` | scattered | Builds warn; fix on touch |
| `GetDBRec()` called on null `this` | `UnitData` and friends | Always null-check before calling |
| Sprite RLE decoders trust file data | `Sprite.cpp`, `SpriteLow565.cpp`, `FacedSprite*.cpp`, `ActorPath.cpp` | Corrupt `.spr` files cause heap overruns; bounds checks are partially in place but **not complete** |
| `MouseThread` / threaded SDL calls | Removed in this fork — do not re-add | macOS will crash; Linux/Windows races |
| `std::list::sort` on pool-allocated nodes | `settlemap` (already fixed) | Watch for the same anti-pattern elsewhere |

See `BUG_HUNT_REPORT.md` for the catalogue (963 entries, categorized by severity).
Don't try to fix everything — fix what crashes or what the user touches.

### Globals you'll see everywhere

`g_player[]`, `g_theWorld`, `g_theUnitPool`, `g_theArmyPool`, `g_theCityPool`,
`g_theGameSettings`, `g_slicEngine`, `g_turn`, `g_selected_item`, `g_rand`,
`g_theProfileDB`, `g_theStringDB`, `g_theAdvanceDB`, `g_theUnitDB`, …
All are raw pointers, lifetime tied to a game session. Null between sessions.

---

## 6. Testing & debugging harness

### 6.1 Smoke test (GUI binary + Python over Unix socket)

The game embeds a Unix-socket command server (`ctp2_code/test/smoketest_server.cpp`)
listening on `/tmp/ctp2-smoke.sock`. The Python harness drives it without a GUI.

```bash
# One-shot smoke test (new game → settle → end turn)
mise exec -- python3 ctp2_code/test/smoke_test.py

# Long-form AI-vs-AI run under sanitizers (THE crash hunter)
ctp2_code/test/autoplay_run.sh 100
# → writes /tmp/autoplay-report-<ts>.md with deduped ASan/UBSan/assert hits
```

Socket commands (see `smoketest_server.cpp`): `new_game`, `start_game`,
`enable_autoplay`, `end_turn`, `advance_turns`, `build_city`, `set_production`,
`enable_governor`, `turn_counter`, `save_game`, `load_game`, `screenshot`,
`diplomacy_status`, `move_unit`, `list_visible_units`.

`autoplay_run.sh` env vars:
- `AUTOPLAY_TURNS` (default 100)
- `AUTOPLAY_SAVELOAD_INTERVAL` — exercise serialization paths
- `AUTOPLAY_SCREENSHOT_INTERVAL` — exercise rendering
- `AUTOPLAY_OVERALL_TIMEOUT` (default 2400s)

**Reach for the smoke test before reading code.** It reproduces 90% of "the
game crashes after a turn" in <1 minute and gives you a sanitizer stack trace.

### 6.2 `ctp2_headless` — no-GUI binary for scripted runs

`ctp2_headless` is the workhorse for save/load, determinism, and AI
progression investigations. Same engine, no UI, controlled by CLI flags
instead of a socket.

```bash
# New game, run N turns, export end-state metrics CSV
./build/ctp2_headless --new-game --players 4 --seed 42 --turns 50 \
                     --export-metrics /tmp/out.csv

# Save midgame
./build/ctp2_headless --new-game --players 4 --seed 42 --turns 25 \
                     --save-game /tmp/mid.sav

# Resume from save and continue
./build/ctp2_headless --load-game /tmp/mid.sav --turns 25 \
                     --export-metrics /tmp/resumed.csv
```

Flags: `--new-game`, `--load-game <path>`, `--turns N`, `--players N`,
`--seed N`, `--save-game <path>`, `--export-metrics <path>`,
`--userprofile <path>`. Logs to stderr via spdlog
(`[ts] [headless] [info] …`). Exits 0 on completion.

Profile resolution is `ProfileDB` ctor arg > `CTP2_PROFILE` env >
`~/.ctp2/userprofile.txt` > bundled `profile.txt`. Explicitly selected
profiles are never written back, and headless never autosaves nor
writes `userprofile.txt`. Test harnesses point `CTP2_PROFILE` at the
pinned `ctp2_code/test/testprofile.txt` so results don't depend on the
developer's personal profile.

The metrics CSV has two sections — `# PLAYERS` (idx, leader, dead,
score, gold, num_cities) and `# CITIES` (player_idx, name, x, y,
population). Diff two CSVs to investigate determinism / save-load
divergence.

### 6.3 Test tiers — which to run after which change

| Tier | Command | Time | What it covers |
|---|---|---|---|
| `fast` | `meson test -C build fast` | ~24s | Pure unit (Army, MapPoint, RandGen, observers, color types) + include-direction ratchet tests (`gs/`→`ui/`/`gfx/`). No game spawn. |
| `unit` | `meson test -C build unit` | ~98s | Adds DB load (BuildQueue, CityData, Happy), headless subprocess spawns (smoke/determinism/progression/long), save-load round-trips. |
| `smoke` | `meson test -C build smoke` | ~12s | Launches the GUI binary, drives it over the Unix socket. |
| (all) | `make test-full` | ~2-3min | All of the above. |
| coverage | `make coverage` | ~5-10min | Rebuilds in `build-cov/`, runs tests + headless scenarios, gcovr summary for `gs/+ai/`. Report at `build-cov/coverage-report/index.html`. |
| autoplay | `autoplay_run.sh 100` | ~5-20min | Sanitized AI-vs-AI for crashes / ASan hits. |

Rules of thumb for what to run:
- Touched `gs/gameobj/`, `gs/fileio/`, `gs/database/`, or any `Serialize()` → unit + autoplay (and load an existing save).
- Touched `ai/` → unit + autoplay.
- Touched `gfx/` or `ui/` → fast + smoke. Visual regressions need a human eye; the smoke test only catches crashes.
- Touched `gs/` headers (added/removed includes) → fast (ratchet tests live there).
- Added a new test file → fast or unit depending on tier (see `cpp_fast_test_sources` vs `cpp_test_sources` in `ctp2_code/meson.build`).

The `unit` tier currently exercises save/load with `compare_metrics_soft`
(structural REQUIRE/CHECK, AI-decision drift via `MESSAGE("WARN: …")`).
See `BUG_HUNT_REPORT.md` → `SAVE_LOAD_AI_DETERMINISM`.

### 6.4 Local CI daemon (`.ci/`)

A background daemon runs Tier A on every working-tree edit (debounced)
and Tier B on every HEAD change, writing structured state to
`.ci/state.json`. Designed for coding agents — don't invoke
`meson test` ad-hoc when the daemon already has fresh results.

```bash
make ci-start          # start the daemon (nohup; runs until ci-stop)
make ci-status         # cat .ci/state.json
make ci-failures       # cat the most recent failure record
[ -f .ci/STATUS_RED ]  # quick "is master green?" check
```

`.orchestration/bin/merge.sh` and `.easy/bin/review.sh` refuse to act
when `.ci/STATUS_RED` is present — workers can't pile commits onto a
red master. Override with `CTP2_CI_OVERRIDE=1` for intentional
recovery batches. Full design and schema in `.ci/README.md`.

---

## 7. Crash-after-turn workflow (savegame from user)

The user gives you a save that crashes on or after `end_turn`. Decide between
**fix-the-game** (engine bug, save is fine, will help everyone) and
**fix-the-save** (save is corrupt or in a state the engine can't handle, fix
just this one). Default to fix-the-game unless evidence forces otherwise.

### Step A — reproduce deterministically

1. Drop the save under `save/games/<profile>/`.
2. Run the sanitized binary, load it, end turn:
   ```bash
   ./build-sanitized/ctp2 2>&1 | tee test/crashes/repro.log
   ```
   Or scripted via the smoke test:
   ```python
   send_cmd("load_game"); send_cmd("end_turn")
   ```
3. Confirm reproduction. If it's non-deterministic, re-run with
   `g_rand` seed pinned (search `RandGen::Seed` callers) — most CTP2 logic is
   RNG-driven, so an intermittent crash usually means a memory bug, not a
   logic bug.

### Step B — collect signal

- ASan/UBSan stack trace from `repro.log`. First sanitizer hit is usually
  the root cause; later ones are corruption echoes.
- `slicdbg.txt`, `usercritmsgs.txt`, `eventlog.txt`, `ldlparselog.txt` —
  game-side diagnostic logs written to the working dir.
- `ls -la save/games/<profile>/AUTOSAVE-*` — the autosave from **the turn
  before** the crash is gold; it lets you diff state.

### Step C — classify

| Signal | Likely class | Path |
|---|---|---|
| ASan heap-buffer-overflow in `Sprite*` / `RLE` | Bad sprite asset or decoder bug | Fix decoder in `gfx/spritesys/`; the save is fine |
| ASan use-after-free in event/pool code | Engine bug | Fix in `gs/gameobj/` (`*Pool` classes); save is fine |
| UBSan signed-overflow / shift on enum index | Engine bug, enum widening | Bounds-check the accessor; save is fine |
| Null deref `this == 0x0` in `UnitData::GetDBRec` etc. | Stale ID referenced after unit died | Engine bug — null-guard; sometimes the save has an orphan reference (fix-save fallback) |
| Magic-number mismatch / `g_saveFileVersion` rejected | Save from older/newer version | See §8 |
| SLIC assertion in `slicdbg.txt` followed by crash | Script bug (mod or stock) | Patch the SLIC file in `ctp2_data/default/scripts/` |
| Division by zero (UBSan) in city/economy code | Engine bug, edge case in city state | Guard the divisor; usually a one-line fix |

### Step D — fix the engine (preferred)

1. Write a failing C++ unit test in `ctp2_code/test/cpp/test_*.cpp` if the
   bug isolates to one subsystem (CityData, BuildQueue, RandGen tests already
   exist). `doctest` is the framework.
2. Otherwise reproduce via the smoke test as a regression scenario.
3. Apply the minimal fix. Don't refactor surrounding code "while you're there".
4. Rebuild sanitized; rerun the repro; rerun `autoplay_run.sh 50` to make
   sure you didn't regress something else.
5. The user's save now loads — no save modification needed.

### Step E — fix the savegame (fallback)

Only when: the engine fix is risky/large, the user needs to keep playing now,
or the save is genuinely corrupted (truncated, mod removed, etc.).

Savegame format basics:
- Binary, little-endian, **header is ASCII `CTP00xx`** where `xx` is the
  format version (see `GameFile.cpp:160` — current is 66, fork supports up
  to 67 with `USE_FORMAT_67`). The header lets you grep saves.
- Body is a `CivArchive` stream — each subsystem's `Serialize(archive)`
  appends to it, in the order at `GameFile.cpp:340-460`. There is no chunk
  table; offsets are implicit. Skipping one subsystem desyncs everything
  that follows.
- Most saves contain back-pointers to record-DB indices (Unit type N,
  Building type M). If the user changed their mod between save and load,
  indices shift → silent misinterpretation → crash.

Options, easiest first:
1. **Magic-number bump.** If the save is from upstream/civctp2 with a newer
   header and the body is compatible, sometimes editing the 7-byte header
   to a version the fork accepts works. Verify with a sanitized load.
2. **Mod alignment.** Confirm `ctp2_data/default/gamedata/*.txt` matches the
   mod the save was created with. Misaligned `Units.txt` / `Buildings.txt` is
   the #1 cause of "crashes only on this save".
3. **Autosave rollback.** Load `AUTOSAVE-<profile>` from one turn earlier —
   often the post-turn state is what's corrupt, not the pre-turn state.
4. **Surgical edit.** Write a small Python script reading the save as bytes;
   locate the offending struct via known sentinels in nearby strings (player
   names, city names — they're length-prefixed C strings). Patch the bad
   field. This is brittle; do it only for one user, never as a release.
   `fix_gameinit.py` is an existing example of save-side surgery — read it
   before writing a new one.
5. **Last resort.** Use `civ3_main.cpp` `--scenario-extract` paths (if
   present) to dump → re-import. Cheap if it works, throws away history.

Whatever you do at the save level, **also write down what the engine should
have caught** in `BUG_HUNT_REPORT.md` so the real fix isn't lost.

---

## 8. Save compatibility caveats

- Magic-number table is `s_magicValue[]` in `GameFile.cpp:160`. To accept a
  new version, add a row **and** every `Serialize` for changed subsystems
  must branch on `g_saveFileVersion`.
- The fork has **not** adopted upstream civctp2's unified cross-platform
  save format (`22dd180`). Saves are currently only guaranteed portable
  within builds of this fork. Don't assume Linux ↔ macOS save compatibility
  until tested.
- Pool subsystems (`UnitPool`, `ArmyPool`, etc.) serialize as flat arrays
  of slots; slot indices are stable within a session but **not** across
  saves. Cross-references go through `Unit`/`Army` ID handles, not raw
  indices.

---

## 9. Orchestration harness — Claude + opencode/Kimi workers

This repo ships a two-tier orchestration system for batched refactoring
work. A fresh agent should know it exists before opening the source tree.

| Tier | Location | Worker model | Repo | Branch model | Use for |
|---|---|---|---|---|---|
| Heavy refactoring | `.orchestration/` | Claude (Opus) drives, 4–8 Kimi K2.6 workers via opencode | Sibling worktrees of this repo | `oc/refactor-{1..7}`, `oc/tests-1` → cherry-pick to master | Header migrations, mechanical renames, file moves, parallel-safe bounded refactors |
| Easy tickets | `.easy/` | Kimi-only, auto-review | Isolated sibling `ctp2-easy/` | `easy-{1..4}` → cherry-pick later | `H-container`, `L-smell` style mechanical modernisation (NULL→nullptr, missing `override`, range-for, C arrays → `std::array`) |

**Entry points (read before invoking either harness):**
- `.orchestration/PLAYBOOK.md` — 6-step procedure, state.json schema, known pitfalls
- `.easy/PLAYBOOK.md` — 4-step loop, isolation rationale

**Sibling worktrees in `~/projects/llm/games/`** (do NOT delete their
`build/` directories — re-`meson setup` is slow):

```
ctp2/                main repo, master (Claude orchestrator lives here)
ctp2-refactor-{1..7} heavy-refactor workers
ctp2-tests-1         worker 8 (originally tests, repurposed)
ctp2-eval            cherry-pick + build target for review before merge
ctp2-easy/           isolated repo for easy-ticket workflow
```

`.scouts/tickets/` holds the pre-scoped ticket pool (`H-container`,
`L-smell`, etc.) that feeds `.easy/`. Skip `H-memory` (ownership audit
needed) and `H-solid` (design needed) for worker pools.

**When to invoke the harness:** Olek says "spawn N workers", "use
opencode for these refactors", "orchestrate this batch" — and the
backlog is ≥3 bounded, same-shape, parallel-safe tasks. For one-off
work or design tasks, do it in this session directly.

---

## 10. When making changes

- Touch the **minimum** needed. This codebase punishes drive-by refactors —
  silent globals and serialization order break in non-obvious ways.
- Prefer **bounds checks and guards** over restructuring. Most fixes are
  3-5 lines.
- After **any** change to `gs/`, run the sanitized autoplay for ≥50 turns.
  Subtle save/serialization bugs only surface across multiple turns.
- After any change to `gs/fileio/`, `gs/database/`, or any `Serialize()`
  method, **load an existing save** to confirm you didn't break compat.
- Don't add new global state. There's too much already.
- Don't introduce threads or move existing work onto threads.
- Comments: existing code is heavily commented (Activision style). New code:
  comment only where the *why* is non-obvious (matches Olek's general rule).
- **Names explain themselves.** Methods, variables, types, test helpers —
  pick names where intent is clear without chasing definitions. Prefer
  `compare_metrics_soft` over `cmp_m`, `find_headless_binary` over
  `find_bin`, `is_pre_save_city` over `flag1`. The legacy 1999-era
  abbreviations (`c3`, `slic`, `aui_`, `g_the*`) are part of the codebase's
  identity — leave them alone — but don't propagate that style into new
  code.
  - **Avoid single-letter locals and loop variables** (`c`, `f`, `t`, `r`).
    They only shrink the file; they make code harder to read and invite
    real bugs — e.g. reusing `c` as a loop variable silently clobbered a
    `c` holding query results one scope up, crashing the recorder. Use
    `city`, `frame`, `token`, `response`. A short index `i`/`j`/`x`/`y` in a
    tight numeric loop is fine; a single letter standing in for a real
    object or value is not.
- **Param struct + designated initializers when args ≥ 5 or any duplicated
  primitive type.** Functions taking ≥5 args, OR any 2+ args sharing a
  primitive type (`sint32` + `sint32`, two `bool`s) where order ambiguity
  could cause silent bugs, take a `const ParamStruct&` instead of
  positional args. Call sites use C++20 designated initializers:
  ```cpp
  void Resize(const ResizeParams& p);
  Resize({.width = 800, .height = 600, .keep_aspect = true});
  ```
  Strongly-typed payload types that already exist (`BattleEvent`,
  `Response`, `Message`) follow this rule automatically — pass them by
  `const&`. Does NOT apply to ≤4 args of distinct types;
  `Draw(surf, x, y)` stays positional. Requires the C++20 `cpp_std`
  setting in `ctp2_code/meson.build`.
- **Commit messages use `type(scope): summary`** — `fix(gfx/spritesys): …`,
  `refactor(gs/gameobj): …`, `test(headless): …`, `feat(gs/core): …`,
  `docs(agent): …`. Scope is the directory or subsystem touched. Summary
  is imperative and under ~70 chars. The orchestration scripts enforce
  this convention on worker commits; humans should match.
- **Definition of done = the daemon agrees.** Before declaring a task
  complete (or before the user has to ask), check
  `jq -r '.tier_a.status' .ci/state.json`. If `green`, you're done.
  If `running`, wait — the daemon is faster than re-running tests
  yourself, and re-running burns build cache the daemon was about to
  use. If `red`, read `.ci/failures/$(ls -t .ci/failures/ | head -1)`
  for a structured `{test, file:line, message, info}` summary and
  address it. After committing, re-check `tier_b.status` the same way.
  The agent's job ends when the daemon's status agrees, not when the
  edit was written. See `.ci/README.md` for the full schema. If
  `make ci-status` shows the daemon hasn't run anything yet, start it
  with `make ci-start`.

---

## 11. North star — where this fork is going

These goals shape every change. Weigh them against the §10 "touch the
minimum" rule with judgment, not zeal. When a small decision could go
either way, pick the option that moves toward one of these and doesn't
move away from the others.

1. **Modern, idiomatic C++ end state.** Target is C++20/23 throughout:
   RAII, smart pointers, `std::optional` / `string_view` / `span`,
   ranges, structured bindings, scoped enums. No raw `new` / `delete`
   in new code; no new manual ownership. Leave touched subsystems more
   modern than you found them — even slightly. The legacy MFC-era
   patterns get displaced one subsystem at a time, not in drive-by
   passes.

2. **Layered, intent-revealing architecture.** The endpoint is a
   codebase worth pointing at as a clean example of a long-lived game
   engine — explicit ownership, narrow interfaces, enforced dependency
   direction (the `gs/` → `ui/` / `gfx/` ratchet tests are the
   current example). Avoid clever; prefer obvious. Apply patterns
   when they fit, not for their own sake.

3. **Test coverage that earns its keep.** Every non-trivial change
   ships with a test that would have caught the regression. Prefer
   headless integration tests (catch systemic bugs across subsystems)
   over over-mocked unit tests. The coverage trend should go up;
   absolute numbers are secondary.

4. **Moddable by design, before mods exist.** No mods are planned,
   but public-facing surfaces (record DB schemas, SLIC bindings, the
   save format, asset paths) should be explicit, documented, and
   stable as if a modder were already depending on them. No implicit
   ordering, no magic numbers without source comments, no hidden
   globals a modder would have to monkey-patch. A future modder
   should be able to answer "where do I plug in X?" from the schema,
   not from spelunking the source.

### Current state (snapshot — verify against `git log` if stale)

The live picture lives in `~/projects/claude/plans/ctp2.md`. As of
2026-05-28:

- **UnitActor split** is mid-flight (Phase 3 slice 4 just landed —
  `UnseenCell` owns its snapshot `UnitState`). One or two more
  slices expected before Phase 3 closes.
- **Local CI** (`.ci/`) is in design — see the test-tier section §6.3.
- **Save format rework** (planned: structured, human-readable JSON
  schema; addresses `SAVE_LOAD_AI_DETERMINISM`) is queued behind
  finishing the UnitActor split and getting local CI in place.

When picking up a session cold, run `git log --oneline -10` first —
this section ages fast.

Update this section when a phase closes or a planned item starts.

---

## 12. Quick reference

| Need | Path |
|---|---|
| Where's the turn loop? | `gs/gameobj/Player.cpp` `BeginTurn`/`EndTurn`; orchestrated by `civapp.cpp` |
| Where are units serialized? | `gs/gameobj/UnitData.cpp` `Serialize` + `UnitPool::Serialize` |
| Where's the AI top level? | `ai/ctpai.cpp`, `ai/mapanalysis/`, `ai/strategy/goals/` |
| Where's the sprite decoder? | `gfx/spritesys/Sprite.cpp` (`ConvertPixelFormat`) |
| Where's the SLIC VM? | `gs/slic/SlicEngine.cpp` + generated `slic.tab.c` / `lex.slic.c` |
| Where are sound assets played? | `sound/civsound.cpp` |
| Game start path | `ctp/civ3_main.cpp` → `CivApp::InitializeApp` → `civapp.cpp` |
| Where saves get written | `gs/fileio/GameFile.cpp` `GameFile::SaveGame` / `LoadGame` |
| Where crashes get logged (runtime) | `test/crashes/` (via `run_game.sh`); also `slicdbg.txt`, `eventlog.txt`, `usercritmsgs.txt` in CWD |
