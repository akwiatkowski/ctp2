# CTP2 Refactoring Plan

Purpose: give any assistant or developer a quick, shared answer to: "how much more refactoring work remains?"

Plain Markdown so any LLM (OpenAI, Claude, local models) can read and update it. Keep it short. Update after each committed batch.

## Overall Progress

- **Original Definition of Done (M1–M8): complete.** Only the P7 documentation close-out (~1 session) remains before declaring it formally.
- **Current phase: memory safety & stability**, tracked as work items **P1–P7** below, sorted by severity/importance (highest first; ordered 2026-07-02).
- **Progress:** P1 complete (all 54 CRITICAL verified). P3 underway — 2 of the 4 HIGH crash-class categories done (**DANGEROUS_SHIFT + DIVISION_BY_ZERO**, both `0 open` first-party); MISSING_BOUNDS_CHECK is started (bounds mini-batches through `profileDB` verified; 14 open sites fixed, several stale/false-positive leads annotated); NULL_DEREFERENCE started with obvious local guards through `UnitActor`/`directorevent`. P4 started (`unsafe_string_api` 1415→1410); P6 ticked 11/574. Key finding: `BUG_HUNT_REPORT.md` HIGH findings are stale leads — line numbers drifted, most already fixed or false-positive; **verify by code pattern, not line number**.
- **Parked:** modern-asset converter (formerly M10).

Recount remaining work any time with:

```sh
grep -c '^- \[ \]' REFACTORING_PLAN.md                 # open items here
grep -c '^- \[ \]' docs/memory-refactor-checklist.md   # P6 (M11) files remaining
```

## Short Answer Formula

When asked "how much more work remains?":

- Declare the original Definition of Done (P7 docs pass): **~1 session**.
- Crash-class stability work (P1–P5): **bounded, ~5–10 focused sessions** (P1 done; P3 ~half through its HIGH categories — the two large buckets, ~470 bounds/null findings, are the bulk of what's left, though most are expected already-fixed on verification).
- Mechanical RAII conversion (P6): **multi-session; ~413 files, each needing ownership analysis (no free mechanical tier — verified 2026-07-02)**.
- Fully refactored engine: **open-ended / not a bounded goal**.

## Completed Milestones (compact)

| ID | Milestone | Result |
| --- | --- | --- |
| M1 | Baseline safety loop | ratchets enforced in `make test`; `make ubsan-smoke` works; ASan blocked on macOS (see P2) |
| M2 | Warning noise reduction | 8/8 batches done; leftover categories under Caveats |
| M3 | Modernization ratchet burn-down | 6/6 done; baseline in `tools/modernization/ratchet_baseline.json`, current counts in P6 snapshot |
| M4 | Timelapse/play tooling polish | 5/5 done; long fogged timelapse, captions, `docs/timelapse.md` |
| M5 | UI/frame polish | SDL idle cap uses a 17 ms frame budget (`a3e59cf6`) |
| M6 | Final stabilization pass | 4/4 done; tests green, README build docs corrected |
| M7 | Obsolete subsystem removal | 8/8 done; CD-ROM/GameWatch/registry checks/DirectX backend deleted; movie + network code retained (see Caveats) |
| M8 | Modern asset pipeline spike | 5/5 done; `.SPR` format decoded read-only (`tools/assets/spr_inspect.py`, `spr_export.py`); 151/151 v0/v1 unit sprites pass parity check; design + licensing in `docs/modern-assets.md` |

Detailed notes live in git history (e.g. `git log --oneline --grep 'M7'`), the tools' source comments, and the referenced docs.

### Caveats retained from completed work

- **ASan/macOS blocker (M1) — root cause pinned 2026-07-02:** the hang is a **re-entrant deadlock in the ASan runtime on macOS 26 (Tahoe)**, and affects **every** ASan binary on this machine — a 6-line `malloc`+overflow test hangs pre-`main` under both Apple clang 17 *and* Homebrew LLVM 21, so it is **not** Apple-clang- or CTP2-specific. Stack: `AsanInitFromRtl` holds the init spin-lock → `InitializeShadowMemory` → `MemoryRangeIsAvailable` → `get_dyld_hdr()` allocates via the malloc-zone interceptor (`__sanitizer_mz_malloc`) → re-enters `AsanInitFromRtl` → blocks on the lock it already holds (`StaticSpinMutex::LockSlow`). Tahoe's dyld changed `get_dyld_hdr` to allocate during init; older macOS did not. No `ASAN_OPTIONS` / `MallocNanoZone=0` / link-order flag fixes it (all tried). `make ubsan-smoke` is the working sanitizer tier; **P2's only viable path is Linux** (local Docker/Colima — daemon not currently running — or CI).
- **Remaining warning categories (M2; post-DoD, opportunistic only):** include-case stragglers outside the swept paths; writable-string-literal conversions (`char *`/`MBCHAR *`) in diplomacy/UI/logging; unused-variable warnings in `governor.cpp`/`ArmyData.cpp`/`diplomat.cpp` (initializers may have side effects — review before removal); `&&`/`||` precedence in `ArmyData.cpp`/`robotastar2.cpp`; switch-exhaustiveness and overloaded-virtual in UI/sprite/network classes.
- **Do not remove (M7):** network/multiplayer code (resolve later); movie playback code and wonder/victory movie DB/schema/data; the DirectShow movie reference `ui/aui_directx/aui_directmovie.*` + `aui_directmoviemanager.*` and their header closure `aui_directui.h`/`aui_directsurface.h`/`aui_directx.h` — kept for a future SDL-based movie repair, never compiled on SDL builds.
- **CRLF project files (M7):** legacy `.dsp`/`.mak` files use DOS line endings; `git diff --check` "trailing whitespace" on their changed lines is the pre-existing CR byte — do **not** "fix" it, stripping the CR corrupts the VS6 format. The VS6/autotools project files are not the canonical build (Meson is).
- **Assets are user-supplied (M8):** game data is not part of the Activision/Apolyton source release. Generated modern assets are local, rebuildable cache under `~/.ctp2/assets/<fingerprint>/` — never committed or redistributed; original data stays canonical.

## Active Work — sorted by severity/importance

| P | Item | Status | Size |
| --- | --- | --- | --- |
| P1 | Remaining CRITICAL crash bugs (BUG_HUNT_REPORT) | ✅ 1/1 done | complete |
| P2 | ASan smoke tier on Linux | 0/1 | ~1 session |
| P3 | HIGH-severity findings, category batches | DANGEROUS_SHIFT + DIVISION_BY_ZERO done (1st-party); MISSING_BOUNDS_CHECK started through `profileDB`; NULL_DEREFERENCE started through obvious `UnitActor`/`directorevent` guards | multi-session |
| P4 | Unsafe string APIs (`unsafe_string_api=1410`) | started | multi-session |
| P5 | JSON-load derived-cache audit | 0/1 | ~1 session |
| P6 | Mechanical RAII conversion (was M11) | 11/574 files | multi-session |
| P7 | Close-out docs, declare DoD (was M9) | 0/2 | ~1 session |
| — | Modern-asset converter + first-run (was M10) | **parked** | deferred |

Why P1–P5 outrank P6: raw `new`/`delete` → RAII mostly prevents **leaks**, which rarely hurt a play session. Out-of-bounds indexing, null derefs, division by zero, UB shifts, and unsafe string writes are what actually crash or corrupt the game — and the repo already has them catalogued. P6 stays active and interleaves well (same worker fan-out pattern), but crash-class fixes deliver more player-visible stability per line changed. P7 is tiny and fine to slot in anytime as a warm-up; it is last only because it is documentation, not code.

### P1 — Verify & fix remaining CRITICAL crash bugs

`BUG_HUNT_REPORT.md` (May 2026) catalogues **963 static-analysis findings** (54 critical, 441 high, 449 medium). Only ~63 were ever fixed (commits `c694afe3`, `970d9bec`, `d7e9e0ff`, `33211c11`, all 2026-05-12); nothing since, and the report has no fixed/open tracking. Findings are **leads, not verdicts** — verify each in code first; marking one false-positive with a one-line justification counts as progress.

- [x] Verify and fix the remaining CRITICAL findings; annotate the report with fixed/false-positive status per finding so the report stops rotting. **Done 2026-07-02:** all 54 CRITICAL findings verified in code — 49 were already fixed or obsolete, 5 were genuinely open and fixed this pass (commits `fbbe6965` c3cmdline console buffers, `65406bc4` spriteutils DecodeToBuffer, `9aee4932` CityData 64-bit pointer truncation, `2fe812d5` NetUnit/ChunkList packet bounds). Report now carries a resolution-status block at the top of its CRITICAL section. `make test` + `make ubsan-smoke` green.

### P2 — ASan smoke tier on Linux

ASan is the single highest-leverage detector (`MEMORY_SAFETY_STRATEGY.md`): use-after-free, heap/stack overflow, leaks — the classes UBSan does not catch. Native macOS ASan is **impossible on this machine** — confirmed 2026-07-02 as a runtime deadlock on macOS 26 that hangs even a trivial program (see the M1 caveat for the exact re-entrancy chain); Linux ASan is unaffected. This also **guards P6 itself**: a bad RAII conversion introduces double-frees that ASan catches immediately, so it is most valuable *right before* P6 ramps in volume — until then `make ubsan-smoke` covers the shift/overflow/UB classes the crash-class work (P3/P4) targets.

- [ ] Get the ASan smoke run working on **Linux** (native-only — see M1 caveat; the sole prerequisite is a running container engine: start Docker Desktop or `colima start`, then build an arm64 Linux image with `-Db_sanitize=address,undefined`), wire it in as an on-demand `make asan-smoke-linux` target reusing `test/repro.py`, and document the invocation here.

### P3 — HIGH-severity findings, category batches

- [ ] Burn down the ~420 unaddressed HIGH findings from `BUG_HUNT_REPORT.md` in category batches (bounds checks → null derefs → div-by-zero → shifts), one module per commit, each finding verified in code before fixing. Fans out to workers the same way as P6.

**DANGEROUS_SHIFT (first-party) done 2026-07-02.** Verified all HIGH shift findings in code; report line numbers had drifted since May 2026 and most were already fixed (converted to `safe_shift_left_u64` or guarded `< 64`/`< 32`) by prior passes. Genuine open sites fixed this pass across `cellunitlist`, `ArmyData` (17 sites), `armyevent`, `CityData`, `slicfunc`, `UnitActor`, `c3cmdline`: player-index signed-shift UB (`1 << idx` → `1u << idx`, since `k_MAX_PLAYERS==32` makes `1<<31` reachable and UB) and two classes of unbounded shift (DB-record building-mask and raw-`atoi(argv)` console shifts) routed through `safety.h` helpers. Net: **0 open first-party**; net-code shift findings (`net_cheat`, `net_info`, `net_thread`) deferred per ground rules. Report section carries a resolution block. `make build`/`make test`/`make ubsan-smoke` green.

**DIVISION_BY_ZERO (first-party) done 2026-07-02.** Verified every HIGH div-by-zero finding; the vast majority were already fixed by prior passes (`safe_divide`/`safe_divide_double` or explicit `> 0` guards in `CityData`, `FeatTracker`, `Readiness`, `SlicFunc`, `UnitData`, `aui_listbox`, `c3slider`, `director`, `soundmanager`, `UnitActor`). Only genuine open site: `SpriteLow565` scaled-draw `/(double)destHeight` cast-to-int UB at `destHeight==0` (guarded inline). All `Barbarians` findings are **false positives** — `RandomGenerator::Next(sint32)` self-guards `if (r <= 0) return 0`. Net: **0 open first-party**. **Next categories:** MISSING_BOUNDS_CHECK and NULL_DEREFERENCE (the two large buckets, ~305 + ~166 raw, many likely already fixed — verify each).

**MISSING_BOUNDS_CHECK started 2026-07-02.** First mini-batches verified `Agreement`/`Army`/`ArmyData`/`Barbarians`/`Civilisation`/`CivilisationPool`/`Cont`/`GameEventManager`/`Installation`/`Order`/`Regard`/`Player` plus small sprite/UI, `c3cmdline`, and `profileDB` guard clusters: `Agreement.cpp` already used `safe_player`; `ArmyData` empty-army and `CanSlaveRaid` leads were already guarded; `Barbarians` RNG leads are false positives because `RandomGenerator::Next(sint32)` returns 0 for non-positive bounds; `Cont.cpp` and GameWatch files are stale/absent (but `Installation.cpp` was **wrongly** called absent — it exists at lowercase `installation.cpp` and was genuinely open; fixed 2026-07-02, see below); `DiplomaticRequest.cpp` already uses `safe_player`; `Order.cpp` already guards invalid order indices; `agreementmatrix.cpp`, `c3debug.cpp`, and one `c3cmdline` create-unit lead were already guarded. Genuine open sites fixed: `Army::RemoveAllReferences` now uses `safe_player(GetOwner())` before touching the owner player and `player_view::ArmyRemoved` rejects invalid indices at the shared UI-observer boundary; `civilisation_CreateNewPlayer` validates `pi` before assigning `player_arr_Get()[pi]`; `CivilisationPool::Create` fatals before random/DB access when `numCivs <= 0`; `GameEventManager::ProcessHead` validates `m_processingEvent` before indexing `event_description(...)`; `Regard::SetForPlayer`/`GetForPlayer` now guard `player < k_MAX_PLAYERS` before indexing `m_regard`; `Player::SetGovernmentType` now rejects `type == NumRecords()` before DB access; `aui_control`, `SpriteGroupList`, `effectspritegroup`, and `goodspritegroup` now add runtime guards before reported array/string-table accesses; `c3cmdline` diplomacy/trade/tax/workday/wages/rations commands now validate parsed player/city indices before dereference/list access; `profileDB` now bounds the reported profile string copy. NULL_DEREFERENCE mini-batch also started: `aui_tab` rejects null `ldlBlock`, `TaxCommand` validates required args before `argv[]`, `c3errors` uses `abort()` instead of deliberate null writes, `Director::AddMove` guards null actors, `Player` city-transfer paths guard null `CityData`, `directorevent` guards visible-player/effect lookups, and `UnitActor` draw helpers guard the reported null tile/player/image/record lookups before dereference. Continue from the next obvious HIGH bounds/null leads; skip broad `DiplomaticRequestData`, blitter geometry, and network packet-size flows until supervised.

**MISSING_BOUNDS_CHECK second mini-batch (2026-07-02).** Four more genuinely-open sites fixed after verifying the flagged string leads (`MessageData`, `TurnCnt`, `civapp`, `SlicBuiltin`, `SlicEngine`) were already converted to `strlcpy`/`snprintf`: `Pollution::GetPollutionAtRound` off-by-one (`>` → `>=` on a `[k_MAX_POLLUTION_HISTORY]` array); `gameinit_PlaceInitalUnits` clamps `nPlayers` to `k_MAX_PLAYERS` before indexing `g_player`/`player_start_list`; `SpriteStateDB::SetName`/`SetVal` gained release-mode `index`/`m_map` guards (assert-only before); `UnitSpriteGroup::GetHotPoint` guards the post-mirror facing index without breaking the 5–8→3–0 mirror. `make build`/`make test`/ratchet green.

**NULL_DEREFERENCE second mini-batch (2026-07-02).** `WorkWin::Execute` null-checks `selitem_Get()` (returns `g_selected_item`, null pre-init/teardown) before `GetSelectedCity`; `CtpAi_ConsiderNuclearWar` returns early when `player_Get(playerId)` is null. Verified stale/false-positive: `civ3_main` crash-dump `fopen`s already guarded, `gameinit:2464` `new`+`strcpy` safe (global `operator new` `exit(-1)`s, never null), `ctpai:285` inside `#if 0`. `network.cpp`/`slicif.cpp` null leads deferred per ground rules.

**MISSING_BOUNDS_CHECK correction + `installation.cpp` (2026-07-02).** A prior annotation wrongly listed `Installation.cpp` as absent; it exists at lowercase `installation.cpp`. Because `player_Get(i)` does **not** upper-bound `i` (`return g_player ? g_player[i] : nullptr`), `if(GetOwner() >= 0 && player_Get(GetOwner()))` was a genuine OOB read for `GetOwner() >= k_MAX_PLAYERS`; `Installation::RemoveAllReferences` now uses `safe_player(GetOwner())` at both sites. Lesson (again): verify a file's existence/case before annotating "absent."

**MISSING_BOUNDS_CHECK third mini-batch (2026-07-02).** `ProfileDB::DefaultSettings` validates/clamps `m_civIndex` (defaults to 16) against `g_theCivilisationDB->NumRecords()` before `Get()`; `victorymoviewin` guards a possible `-1` from `FindTypeIndex` before `GetMovieFilename`. `profileDB:549` whitespace-trim was already `len > 0`-guarded (stale). The remaining unverified bounds leads are concentrated in the deferred `net_*`/`network.cpp` packet flows and the slic-debug UI `k_AUI_LDL_MAXBLOCK` string buffers (`segmentlist`/`sourcelist`/`watchlist`/`thronedb`/`robotcom`/`tracklen`/`iparser`) — a P4-flavored string-safety cluster for a later pass.

### P4 — Unsafe string APIs

`strcpy`/`strcat`/`sprintf` are the buffer-overflow class — higher real-world risk than the raw-`new` count. Ratchet: `unsafe_string_api=1410`.

- [ ] Convert unsafe-string clusters on crash-prone paths (parsers, UI text, save/load) to `strlcpy`/`snprintf`/`std::string`, lowering the `unsafe_string_api` ratchet baseline per batch.

**Started 2026-07-02.** `ControlTabPanel::AppendBlockName` switched from exact-sized `sprintf` to `snprintf`; `profileDB` now uses bounded `strlcpy` for parsed profile strings; and `c3files_getfilelist` now uses `strlcpy` for fixed-size filename buffers, lowering the ratchet from 1415 to 1410. `c3mem.cpp` also fixed a stale `size_t`/`%ld` format mismatch (`%zu`) but did not affect the unsafe-string counter because it already used `snprintf`. `SlicSegment::GetDescription` now uses its `maxsize` parameter instead of `sizeof(pointer)`. `SlicBuiltin` leader/pronoun and `SlicEngine::AddResearchOnUnblank` report leads were already fixed with `strlcpy`.

### P5 — JSON-load derived-cache audit

Known gap: JSON load skips some derived-cache rebuilds (`SetAllMoveCost`, `CalcChokePoints` unaudited) — wrong-but-not-crashing state after load.

- [ ] Audit the load-path derived-cache rebuilds and pin them with a scenario fixture (`test/` slices are the net).

### P6 — Mechanical RAII conversion (was M11)

Scope (Olek, 2026-07-02): systematically remove legacy manual memory management from the first-party engine — raw `new`/`delete`, `malloc`/`calloc`/`realloc`/`free`, and raw-owning-pointer patterns — moving ownership to RAII (`std::unique_ptr`, `std::vector`, `std::string`, containers) without changing behavior.

The per-file worklist lives in **`docs/memory-refactor-checklist.md`** — one checkbox per affected file (574 first-party files, ~6880 raw matches), grouped by module, tagged with a difficulty label. Checklist and commit tags keep the **M11** name.

#### Progress snapshot (2026-07-02)

Authoritative metric is the modernization ratchet (enforced by `make test`); per-file `rg` counts are noisy size hints.

| Ratchet counter | Phase start | Now | Removed |
| --- | --: | --: | --: |
| `raw_new` | 5528 | **5472** | 56 |
| `raw_delete` | 2065 | **2012** | 53 |
| `c_allocation` | 516 | **515** | 1 |

Files ticked: **11 / 574**. (Ratchet runs ahead of ticked files because most touched files still have a harder residual cluster — e.g. `Sprite.cpp`, the sprite-group family. `ControlTabPanel.cpp` fixes a real `new[]`/scalar-delete mismatch but keeps one explicit `new[]` inside `unique_ptr<MBCHAR[]>`, so raw `new` does not drop; `scoretab.cpp` removed one raw `new`/`delete` pair.)

> **"Easy" tier — heuristic empty, but real easy conversions exist (2026-07-02).** A `/goal` run first inspected all 8 files the grep heuristic tagged 🟢 easy — **every one was a false positive**: transfer-to-sink (`Execute`/`AddEvent`/`InsertItem`/pool), a reference-counted `SlicObject` (`AddRef`/`Release` — `unique_ptr` would double-free), or a global singleton (`theKeyMap`, `wormhole_Get/Set`); those 8 were reclassified (2→hard, 4→medium, 2→skip). A **tightened finder** (local var, deleted by name, never passed to a function / stored to member-global / returned / refcounted / `static`) then surfaced the *genuine* easy set: **5 files converted** (`SlicButton`, `governor`, `c3cmdline`, `loadsavemapwindow`, `loadsavewindow` — 7 `new` + 7 `delete`, also fixing leaks on early-return and `#if`-excluded paths; commit `99755456`), with 2 finder false-positives excluded (commented-out code; `static` pointers transferred to a dropdown behind a cast). Lesson: grep can't tell "delete-on-failure, transfer-on-success" from a real local owner — but a strict no-transfer/no-static/no-refcount filter *can* isolate the truly-mechanical ones. See memory `feedback_memrefactor_easy_triage`.

#### Difficulty breakdown (heuristic triage; legend in the checklist)

**easy** = self-contained local owner; **medium** = single-owner member / needs analysis; **hard** = factory returns crossing callers; **very hard** = linked lists, `void*` handoffs, member pointer arrays, mixed `new[]`/`malloc`, refcounted types; **skip** = game-lifetime singletons, pools/arenas — usually correct as-is, converting adds risk for no gain.

| Difficulty | Files | Unsafe lines | Notes |
| --- | --: | --: | --- |
| 🟢 easy | 0 | 0 | empty — verified false-positive tier (see box above) |
| 🟡 medium | 411 | 3395 | the bulk; read each, confirm one owner, convert per-cluster |
| 🔴 hard | 4 | 96 | change return type, let the compiler guide callers |
| 🔴🔴 very hard | 45 | 742 | supervised (Opus); reason-tagged in checklist |
| ⚪ skip | 103 | 2617 | don't mechanically convert |
| ✅ done | 11 | 30 | committed this phase |
| **total** | **574** | **6880** | |

**Realistic finish line:** `skip` (+ much of `very hard`) should be *encapsulated behind clear owners*, not rewritten. The productive target is 🟡 medium ≈ **3403 lines / 413 files**, each needing ownership analysis (no mechanical tier); the 🔴 49 files are the supervised tail.

#### Where the work is (per-module difficulty × unsafe-line total)

| Module | easy | med | hard | v.hard | skip | lines |
|---|--:|--:|--:|--:|--:|--:|
| `ui/interface` | 0 | 52 | 1 | 17 | 48 | 1413 |
| `gs/gameobj` | 0 | 79 | 0 | 2 | 12 | 938 |
| `gs/slic` | 0 | 14 | 2 | 1 | 4 | 927 |
| `ui/netshell` | 0 | 27 | 0 | 1 | 1 | 518 |
| `ui/aui_ctp2` | 0 | 49 | 0 | 3 | 5 | 469 |
| `net/general` | 0 | 22 | 0 | 0 | 6 | 396 |
| `ui/aui_common` | 0 | 32 | 0 | 5 | 1 | 288 |
| `gfx/spritesys` | 0 | 5 | 0 | 9 | 0 | 238 |
| `test/cpp` | 0 | 18 | 1 | 0 | 1 | 221 |
| `gs/utility` | 0 | 9 | 0 | 0 | 1 | 195 |
| `gs/fileio` | 0 | 4 | 0 | 0 | 4 | 155 |
| `ctp` | 0 | 5 | 0 | 0 | 3 | 142 |
| `gs/world` | 0 | 11 | 0 | 0 | 3 | 126 |
| `ctp/ctp2_utils` | 0 | 8 | 0 | 0 | 1 | 120 |
| `gfx/tilesys` | 0 | 4 | 0 | 5 | 0 | 93 |
| `gs/database` | 0 | 10 | 0 | 0 | 1 | 64 |
| _(other 26 modules)_ | 0 | 84 | 0 | 2 | 12 | ~477 |

`gs/slic` and networking (`ui/netshell`, `net/general`) carry big line counts but are **deferred** (generated/wire code); `gs/newdb`/`gs/dbgen` should be fixed at the generator, not the output.

#### 🟢 Easy files — none

The heuristic 🟢 tier was verified empty (see the box above); there is no trivial mechanical
tier. Start instead with the **smallest 🟡 medium clusters that have a single, in-file owner**
(dtor `delete m_x` paired with one `m_x = new`) — those are the closest thing to easy and are
what the classifier over-eagerly flagged. Confirm ownership per file before converting.

#### 🔴 Hard + very-hard tail — supervised (49, biggest first)

`ui/netshell/netfunc.cpp` (63/14/1) · `ui/interface/sciencewin.cpp` (55/18/0) · `gfx/spritesys/spritefile.cpp` (45/2/0) · `ui/interface/loadsavewindow.cpp` (15/40/0) · `ui/aui_common/aui_ldl.cpp` (32/13/0) · `ui/interface/spriteeditor.cpp` (9/26/0) · `gs/slic/slicif.cpp` (6/3/17) · `ui/interface/spnewgamescreen.cpp` (21/2/0) · `gfx/tilesys/tileutils.cpp` (15/5/7) · `ui/interface/scenarioeditor.cpp` (19/9/0) · `gs/slic/SlicStruct.cpp` (12/8/0) · `gfx/spritesys/UnitSpriteGroup.cpp` (15/13/0) · `ui/interface/EditQueue.cpp` (18/8/0) · `gfx/tilesys/tileset.cpp` (9/13/0) · `test/cpp/doctest.h` (13/33/0) · `ui/aui_ctp2/chart.cpp` (10/7/0) · `gfx/spritesys/Sprite.cpp` (3/10/4) · `ui/interface/greatlibrary.cpp` (11/5/0) · `ui/interface/loadsavemapwindow.cpp` (10/6/0) · `gfx/spritesys/effectspritegroup.cpp` (9/7/0) · `gfx/spritesys/goodspritegroup.cpp` (8/8/0) · `gs/slic/SlicBuiltin.h` (7/0/0) · `ui/interface/dipwizard.cpp` (4/8/0) · `ui/interface/diplomacywindow.cpp` (3/9/0) · `ui/interface/controlpanelwindow.cpp` (7/4/0) · `ui/aui_common/aui_ranger.cpp` (5/6/0) · `gfx/spritesys/FacedSpriteWshadow.cpp` (0/8/3) · `net/io/net_anet.cpp` (5/5/0) · `ui/interface/chatbox.cpp` (4/4/0) · `gfx/spritesys/FacedSprite.cpp` (0/8/0) · `gfx/tilesys/workmap.cpp` (1/6/0) · `ui/interface/DiplomacyDetails.cpp` (3/3/0) · `ui/interface/trademanager.cpp` (4/1/0) · `ui/interface/intelligencewindow.cpp` (4/0/0) · `ui/interface/UnitControlPanel.cpp` (2/2/0) · `ui/interface/unitmanager.cpp` (4/0/0) · `gfx/spritesys/spriteutils.cpp` (2/0/2) · `gfx/spritesys/SpriteGroup.cpp` (0/4/0) · `gfx/tilesys/BaseTile.cpp` (2/2/0) · `gfx/tilesys/resourcemap.cpp` (1/3/0) · `ui/interface/c3dialogs.cpp` (2/1/0) · `ui/aui_common/aui_tab.cpp` (2/1/0) · `ui/aui_common/aui_textbase.cpp` (2/1/0) · `ui/aui_ctp2/ctp2_menubar.cpp` (2/0/0) · `ui/aui_common/aui_win.cpp` (1/1/0) · `gs/events/GameEventArgument.cpp` (1/1/0) · `gs/gameobj/AgreementData.cpp` (5/1/0) _(refcounted SlicObject)_ · `gs/gameobj/Pollution.cpp` (2/1/0) _(refcounted SlicObject)_ · `ui/aui_ctp2/c3_updateaction.cpp` (1/0/0)

Recompute this breakdown after big clusters land: re-run the triage classifier over the checklist and refresh the ratchet snapshot from `tools/modernization/ratchet_baseline.json`.

### P7 — Close-Out docs (was M9)

Smallest item; fine to do anytime as a warm-up — last here only because it is documentation, not code.

- [ ] Write a "Remaining Legacy-Risk Areas" section in this file consolidating: leftover warning categories, ratchet counts, deferred systems (network, movies, Windows project files), and M7/M8 caveats.
- [ ] Final verification pass (`make test`, `make ubsan-smoke`, clean worktree); update Overall Progress and declare the original Definition of Done met.

### Parked — Modern-asset converter + first-run (was M10)

Offline converter (packed atlas + JSON manifests into `~/.ctp2/assets/<fingerprint>/`) plus an engine modern-first loader with legacy fallback. See `docs/modern-assets.md`. Resume only after the memory-safety phase is well underway.

## Ground Rules (P1–P6)

- **First-party only.** Vendored `libs/**` (anet, freetype, tiff, zlib, miles, …) are upstream code — do not refactor them.
- **Deferred sub-areas stay low priority:** networking (`net/**`, `ui/netshell/**`) per M7; generated DB code (`gs/newdb`, `gs/dbgen`) gets fixed at the generator, not the output. Tests (`test/cpp`) are optional cleanup.
- **One file or one clear ownership cluster per commit.** Preserve behavior exactly; no drive-by logic changes.
- **Verify every batch** with `mise exec -- make test` (plus `make ubsan-smoke` when touching headless/game-loop code), then **lower the ratchet baseline** (`tools/modernization/ratchet_baseline.json`) so reductions can't regress.
- Prefer the smallest, clearest-ownership clusters first; confirm real ownership before editing (crude `rg` counts include comments/strings/placement-new noise). Ticking a file whose remaining matches are all non-ownership noise is fine.
- Bug-report findings (P1/P3) are static-analysis **leads, not verdicts** — verify each in code; false-positive + one-line justification counts as progress.

## Progress Rules

- A normal `continue` should complete one checkbox or one coherent part of a checkbox.
- Update Overall Progress and the Active Work status column in the same commit as the change.
- If a task discovers a blocker, add or update a blocker/caveat instead of pretending progress happened.
- Prefer many small commits over one broad refactor commit.

## Assistant Protocol

For any LLM continuing this work:

- Read this file first, then `git log --oneline -10`, then `git status --short`.
- Work the Active Work list top-down (P1 highest) unless Olek redirects; one small batch per commit.
- Verify with `mise exec -- make test` and `git diff --check`; add `mise exec -- make ubsan-smoke` after touching headless/game-loop code.
- Do not treat "fully refactored" as the goal unless Olek explicitly redefines scope.
- Do not make converted assets canonical; original data/mod compatibility stays; generated modern assets are rebuildable cache.
- Warning cleanup and opportunistic ratchet reductions outside P4/P6 are side batches only.

## Last Known Verification Commands

```sh
mise exec -- make test
mise exec -- make ubsan-smoke
```

## Known Blockers / Caveats

- ASan hangs pre-`main` on this macOS/Apple clang setup (see Caveats); `make ubsan-smoke` is the working sanitizer path until P2 lands the Linux tier.
- The local CI daemon (`.ci/daemon.sh`) is **not running** and `.ci/state.json` is stale (last update 2026-06-07, branch `wave1-ctp2-namespace`). Restart it before relying on `jq '.tier_a.status' .ci/state.json`, or agents will read a month-old green.
- `BUG_HUNT_REPORT.md` has no fixed/open tracking — 963 findings, ~63 fixed in May 2026, no annotations since. P1's checkbox includes fixing that.
- Windows is best-effort and should not drive refactoring decisions unless Olek asks.
- Avoid `MBCHAR *` UI string-literal cleanup unless intentionally doing a broader UI const-correctness batch.
