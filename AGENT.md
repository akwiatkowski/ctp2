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

## 6. Smoke test harness — your primary debugging tool

The game embeds a Unix-socket command server (`ctp2_code/test/smoketest_server.cpp`)
listening on `/tmp/ctp2-smoke.sock`. The Python harness drives it without a GUI.

```bash
# One-shot smoke test (new game → settle → end turn)
mise exec -- python3 ctp2_code/test/smoke_test.py

# Long-form AI-vs-AI run under sanitizers (THE crash hunter)
ctp2_code/test/autoplay_run.sh 100
# → writes /tmp/autoplay-report-<ts>.md with deduped ASan/UBSan/assert hits
```

Commands implemented (see `smoketest_server.cpp`): `new_game`, `start_game`,
`enable_autoplay`, `end_turn`, `advance_turns`, `build_city`, `set_production`,
`enable_governor`, `turn_counter`, `save_game`, `load_game`, `screenshot`,
`diplomacy_status`, `move_unit`, `list_visible_units`.

Useful env vars for `autoplay_run.sh`:
- `AUTOPLAY_TURNS` (default 100)
- `AUTOPLAY_SAVELOAD_INTERVAL` — exercise serialization paths
- `AUTOPLAY_SCREENSHOT_INTERVAL` — exercise rendering
- `AUTOPLAY_OVERALL_TIMEOUT` (default 2400s)

**Reach for the smoke test before reading code.** It reproduces 90% of "the
game crashes after a turn" in <1 minute and gives you a sanitizer stack trace.

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

## 9. When making changes

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

---

## 10. Quick reference

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
