# CTP2 Refactoring Plan

Purpose: give any assistant or developer a quick, shared answer to: "how much more refactoring work remains?"

Plain Markdown so any LLM (OpenAI, Claude, local models) can read and update it. Keep it short. Update after each committed batch.

## Overall Progress

- **Original Definition of Done (M1–M8): COMPLETE and formally declared 2026-07-10.** The P7 close-out documentation pass has landed (the "Remaining Legacy-Risk Areas" consolidation below), and a final verification pass (`make test` + `make ubsan-smoke` + integration/scenario suites, clean worktree) is green. Nothing remains to declare the original DoD met.
- **Current phase: mechanical modernization loop (2026-07-17, /goal-driven) — ratchet burn-down, clarity renames, small safe modernization.** **P11 GPU rendering Stage 2 is COMPLETE and default-on as of 2026-07-17**: buttery trackpad pan (ADR-002), hardware cursor, pinch zoom all ship on a stock launch. The modern-asset sprite track remains the open P11 tail. Memory safety & stability remains open (P2 Linux ASan, P6 RAII).
- **Modernization Phase 2 scoped 2026-07-12** (analysis pass: modern / safe / fast-on-modern-machines): **P8** perf build tier — measured **5.3× wall / 8.6× CPU** from a release build (every existing tier was `-O0`+`-ftrapv`; outcome bit-identical, so optimization does not break determinism); **P9** container modernization (954 legacy `PointerList`/`DynamicArray`/`SimpleDynamicArray` uses); **P10** type-erased-cast burn-down (2487 after redundant file-I/O and binary-read cast cleanups); **P11** GPU rendering Stage 2 un-parked with M10 on 2026-07-14. Verified-already-modern list added so items don't get re-proposed (C++20, hardening max, UBSan tier, arena-pooled A*, JSON saves).
- **Progress:** P1 complete (all 54 CRITICAL verified). **P5 complete 2026-07-10** — JSON-load derived-cache audit: `SetAllMoveCost`/`CalcChokePoints` need no rebuild (outputs are dense-serialized per-cell); the only non-serialized world caches (continent size/neighbor arrays) were already rebuilt in `World::from_json`; one real gap found + fixed — stale `good_value` on a `ResourceDB`-size mismatch now recomputes via `ComputeGoodsValues`, pinned by a new regression test that fails without the fix. **P3 crash-class burned down 2026-07-02/03** — all 4 HIGH categories worked: DANGEROUS_SHIFT + DIVISION_BY_ZERO (`0 open` first-party), and MISSING_BOUNDS_CHECK + NULL_DEREFERENCE (`@~675`, `@~1206`; ~305 leads) verified in-code with resolution blocks — ~25 genuine crash bugs fixed, the rest already-fixed by the `g_player→safe_player` migration, false-positive, structurally-safe, or the supervised/P4/net tail. (A tier-drift found 2026-07-02, where earlier annotations landed in the MEDIUM mirror, was corrected.) **P4 effectively complete** — ratchet re-scoped to first-party (vendored libs excluded), then **85 real conversions drove `unsafe_string_api` 92 → 57** (the old libs-inclusive 1360 was 93% vendored noise); one real overflow bug caught (`tech_MemMap::GetFileExtension`). The clean pool is exhausted — the remaining 57 was dead code (~20), deferred `net/` (~22), and the off-limits `GetXName` idiom / hard cascades (~15); **the dead code was deleted 2026-07-12 (57 → 39, plus a live `GetLabel` `sizeof(ptr)` truncation bug found and fixed in the process — P4 now fully complete)**. P6 ticked 11/574 (17/574 as of 2026-07-11). Key finding: `BUG_HUNT_REPORT.md` HIGH findings are stale leads — line numbers drifted, most already fixed or false-positive; **verify by code pattern, not line number** — and **check the severity tier** before annotating.
- **Test-coverage expansion 2026-07-13** (from a coverage brainstorm; all six picked items shipped): scenario suite grew **5 → 9 tests** — seed sweep (suite no longer lives on seed 42), combat-invariant matrix, forced undersea city (new `create_unit` DEBUG verb; `grant_advance` now takes `ADVANCE_*` names), the **first test ever to finish a game** (conquest ending recognized), and deep-state save/load + economy-corruption invariants folded into the long-game soak. **The deep-state oracle caught a real crash on its first run** — in-process `load_game` of any save with live trade routes aborted in pool teardown (`Game::Set*Ptr` uses `unique_ptr::reset`, which installs the new pool *before* the old dtor runs, so `~TradePool`'s gameplay route-Kill cascade hit the fresh empty pool). Fixed at three layers: dtor cascade removed, `~CityData` route-kills guarded via null-safe `TradeRoute::IsValid`, trade-before-units pool re-init order (commit `d2e14d94`). Unpicked cheap wins: map-generator×wrap sweep (~1h) and a cross-tier determinism diff (debug vs release metrics = automatic UB net, technique proven during P8).
- **Un-parked 2026-07-14:** modern-asset converter (formerly M10), required for P11 Stage 2 atlas-backed GPU compositing.
- **P11 Stage 2 shipped 2026-07-16/17.** The buttery pan was parked (glide invisible), then the new pixel-proof discipline found the world GPU layer had been **empty since Stage 2 D** (null routing key) — every parity oracle was structurally blind to it. Fixed routing + UI-layer hole punch, replaced the margin re-render with the ADR-002 window mirror, added the `pan-pixel-proof` integration test (forced-offset shift + streamed sub-tile glide), hardware cursor (software cursor pickups were baking screen-coord ghosts into the UI layer — the "strobing"/"wrong tile" artifacts), pinch zoom via native macOS magnify (SDL3 forwards no trackpad touches), and flipped `CTP2_GPU_LAYERS`+`CTP2_GPU_CAMERA` **default-on** after a green full integration suite. Three latent bugs fixed en route: `ScrollPixels` OOB memset (SIGSEGV), sprite-filename `sizeof(ptr)` truncation, sprite-import quarter-size leaks.
- **Mechanical modernization loop started 2026-07-17 (/goal).** Per-commit fast tests + ratchet baseline updates: `unsafe_string_api` 39→17, `c_allocation` 101→81, clarity renames (`hscroll`→`tileStepX`), dead code deleted. Next targets + off-limits notes in the ratchet snapshot table below.
- **P12 scoped 2026-07-17 (Olek):** single `~/.ctp2` data root — an install script copies all data to `~/.ctp2/original_data/` (verbatim, canonical), converts sprites into `~/.ctp2/assets/`, and the engine relocates its data + save roots there (HOME-expansion in `CivPaths`), so the ~476 MB in-tree `ctp2_data/` can be deleted. **Sequenced after finishing the modern-sprite wiring first.** See the P12 section.

Recount remaining work any time with:

```sh
grep -c '^- \[ \]' REFACTORING_PLAN.md                 # open items here
grep -c '^- \[ \]' docs/memory-refactor-checklist.md   # P6 (M11) files remaining
```

## Short Answer Formula

When asked "how much more work remains?":

- Original Definition of Done (M1–M8): **DONE — formally declared 2026-07-10** (P7 close-out landed).
- Crash-class stability work (P1–P5): **DONE / at practical floor** (P1 done; P3 HIGH crash-class burned down — the four categories are verified with resolution blocks, ~25 genuine bugs fixed, tail is supervised/P4/net; P4 unsafe-strings effectively complete — re-scoped to first-party then 92→57 via 85 real conversions, clean pool exhausted (tail = dead/deferred-net/idiom-cascade); **P5 load-cache audit done — no world-cache gap, one `good_value` fix landed**). **Remaining bounded work: P2 Linux-ASan (gated on starting a container engine).**
- Mechanical RAII conversion (P6): **multi-session; ~407 medium files left, each needing ownership analysis (no free mechanical tier — verified 2026-07-02, re-confirmed 2026-07-11: ~half the heuristic "easy" hits are copy-assign traps, borrowed pointers, transfer, or dead code)**.
- Modernization Phase 2 (P8–P10): **P8 is ~1 session with a measured 5.3×/8.6× payoff waiting; P9 multi-session; P10 opportunistic.**
- P11 GPU rendering: **Stage 1 done; Stage 2 A–F COMPLETE and DEFAULT-ON 2026-07-17** (A 32bpp UI blitters; B world→32-bit; B1 modern-first atlas hook; C GPU fog; D per-layer compositing; E SDL3 default; F smooth camera → **shipped as the buttery pan**: ADR-002 window-mirror world layer, pixel-proven glide via the `pan-pixel-proof` integration test, hardware cursor, pinch zoom via native macOS magnify, edge/key scroll on the same eased camera; `CTP2_GPU_LAYERS=0` opts back to the legacy present). Remaining P11 tail: modern-sprite atlas polish (scaled/mirrored facings, Good/Effect groups, real v2 parity). See the P11 section.
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
| P3 | HIGH-severity findings, category batches | DANGEROUS_SHIFT + DIVISION_BY_ZERO done. MISSING_BOUNDS_CHECK + NULL_DEREFERENCE HIGH tiers **verified & burned down 2026-07-02/03**: ~25 genuine crash bugs fixed, the rest already-fixed (g_player→safe_player migration), false-positive, or the supervised/P4/net tail. Both sections carry resolution blocks. | tail = supervised/P4/net |
| P4 | Unsafe string APIs (`unsafe_string_api=39`, first-party) | ✅ complete — 85 conversions (92→57) + dead-code deletion 57→39 + live `GetLabel` sizeof(ptr) bug fixed. Tail = deferred-net(~22)/idiom-cascade(~17). | complete |
| P5 | JSON-load derived-cache audit | ✅ 1/1 done | complete |
| P6 | Mechanical RAII conversion (was M11) | 17/574 files; ratchet raw_new 4799, raw_delete 1845 | multi-session |
| P7 | Close-out docs, declare DoD (was M9) | ✅ 2/2 done | complete |
| P8 | Performance build tier (release + thin-LTO) | ✅ 1/1 done — `make release` / `release-check`; 8.6–9.3× CPU banked | complete |
| P9 | Container modernization (PointerList/DynamicArray → std) | ratchets live (576+353); burn-down 0/n | multi-session |
| P10 | Type-erased casting burn-down (`type_erased_casting=2487`) | redundant I/O/API cast cleanups: −157 total | opportunistic |
| P11 | GPU-path rendering | **Stage 1 ✅. Stage 2 A–F ✅ DEFAULT-ON 2026-07-17** (buttery pan ADR-002 + pixel proof, hw cursor, pinch zoom, edge/key scroll on the eased camera; `CTP2_GPU_LAYERS=0` opts out). Quads (G) parked. Tail: modern-sprite atlas polish (mirrored/scaled facings, Good/Effect, v2 parity). | tail open on `p11-stage2-gpu-world` |
| — | Modern-asset converter + first-run (was M10) | **un-parked 2026-07-14**; required before atlas-backed GPU compositing | staged |

**Recommended next order (2026-07-13):** the original DoD is declared; crash-class work (P1/P3/P4/P5) is complete or at its practical floor. The remaining moves, best-ROI first:
1. **P2** — ASan-on-Linux; blocked only on starting a container engine (`colima start` / Docker), then one build. User action gates it. Do this before P6/P9 ramp, to catch conversion-introduced double-frees.
2. **Warm-ups (~1–2h each, from the test-coverage brainstorm):** a **cross-tier determinism test** (same seed on debug AND release, diff the metrics CSV — any divergence is optimization-sensitive UB caught automatically; technique proven manually during P8) and a **map-generator × wrap sweep** (10 turns on each builtin generator × wrap flags — 3 of 4 generators are untested).
3. **P6** — the long multi-session RAII effort (~407 medium files left); interleaves well. The 2026-07-11 session banked 9 conversions (−20 raw_new/−20 raw_delete incl. one real `new[]`/`delete` UB fix); the self-contained fast-verify tier is now largely picked over — the rest is cross-module owners and the supervised 🔴 tail, best done after P2's ASan net exists. Heed the `unique_ptr::reset` teardown lesson from `d2e14d94` when converting singleton owners.
4. **P9** — container modernization; same per-cluster discipline as P6, start only after P6 has a stable rhythm (or interleave module-by-module).
5. **P10** — type-erased cast burn-down; opportunistic, lowest crash-relevance.

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

**⚠️ TIER-DRIFT CORRECTION (2026-07-02) — read before claiming P3 bounds/null done.** The `DANGEROUS_SHIFT`/`DIVISION_BY_ZERO` resolution blocks are correctly in the report's **HIGH** tier (`## HIGH Severity`, ~line 278/382) and are genuinely `0-open`. But every "P3 HIGH MISSING_BOUNDS_CHECK / NULL_DEREFERENCE" resolution block written this phase actually landed in the **MEDIUM**-tier mirror (`## MEDIUM Severity`, ~line 2498/2876). The real HIGH-tier sections — `### MISSING_BOUNDS_CHECK` @~675 (**~174 leads**) and `### NULL_DEREFERENCE` @~1206 (**~131 leads**) — carry **no resolution blocks** and have not been systematically verified. The two tiers catalogue overlapping bugs at slightly different line numbers (`ArmyData:1029` HIGH vs `:1027` MEDIUM), so the code fixes largely apply to both, and spot-checks show the HIGH tier is mostly already-fixed/stale like the MEDIUM one (`AgreementData` uses `safe_player`; `C3GameState.cpp` absent). **But bounds/null P3 is NOT closed** — it needs a real HIGH-tier verification pass over ~305 leads (expected mostly-already-fixed, a handful genuinely open — e.g. `ArmyData:7154/7177`, `ArmyEvent:1383` are HIGH-only and unchecked). Realistic size: multi-session, not "~1 hour."

**HIGH-tier MISSING_BOUNDS_CHECK pass started 2026-07-02 (@~675).** Worked the gameobj/gfx/AI index cluster: fixed `MaterialPool`, `MessagePool`, `tileset`, `UnitActor` (m_playerNum/owner), `SelItem` waypoints, `ArmyData::Battle`, `CityData` ring OOB, `gameinit` scenario-player, `SettleMap`, `tiledraw` DB-Get nulls (commits 88fabfc0→e56fd86d). Big structural confirmation: raw `g_player[` indexing is essentially gone (migrated to `player_Get`/`safe_player`), so most of the "g_player no-bounds" leads are already resolved; many others verified already-fixed or false-positive (see report resolution block). **Remaining HIGH bounds:** the string-overflow class (really P4), file-parse bounds (`spritefile`/`aui_image`/`MapFile`/`targautils`/frame-count loops — supervised), `diplomat`/`DiplomaticRequestData`/`WrlEnv`/`DB.cpp` (supervised), and `net_*` (deferred). **The HIGH NULL_DEREFERENCE section (@~1206, ~131 leads) is still untouched.**

**HIGH-tier NULL_DEREFERENCE pass started 2026-07-03 (@~1206).** Same story as bounds — the `g_player[X]->` → `player_Get`/`safe_player` migration already guards most sites; verified swaths already-fixed (AgreementData, ArmyData, FeatTracker, Happy, MessageData→std::string, Unit/UnitData/Player.GiveAdvance, PlayerEvent ×29 safe_player, Regard, SlicSymbol). Genuine stragglers fixed: `TradeOffer`, `TaxRate`, `Agent::FindPath` GetOrder(0), `ArmyEvent` CreateLeader/builtWonders, `Player::ExchangeMap`, `Slic_CreateUnit` dead-owner. Spot-check list resolved 2026-07-03: fixed StringCompare/UnitArmy/Goal-builtWonders/aui_tipwindow; Player alliance + SlicFunc Pillage already guarded; ArmyData/CityData m_owner left as-is (live-entity invariant); c3cmdline/CityInfluenceIterator GetCell misframed (never null in-range). Supervised remainder: Goal `GetCityData` cluster, SlicSymbol m_struct, background/c3fancywindow UI nulls. See report null resolution block. **Both HIGH crash-class sections are now burned down to the supervised/P4/net tail.**

**Genuinely-open first-party fixes this session (real bugs, tier-independent):** `GameEventArgList` va_list walk now never steps `argString` past its `'\0'` terminator; `installation.cpp` + `installationdata.cpp` owner-index derefs now use bounds-checked `safe_player()` instead of lower-bound-only `player_Get()` (which does **not** upper-bound its index). `WrlEnv.cpp` `(x,y)` env-accessor family (`m_map[x][y]` in `IsLand`/`IsRiver`/`SetGood`/… , ~17 methods) is **reclassified supervised-deferred**, not mechanical: the coordinate contract is inconsistent (some use `GetCell(x,y)`, some raw `m_map`) and `IsLand(x,y)`'s own asserts allow y-wrap (`-k_MAP_WRAPAROUND < y`), so a naive uniform bounds guard could break map topology.

**MISSING_BOUNDS_CHECK correction + `installation.cpp` (2026-07-02).** A prior annotation wrongly listed `Installation.cpp` as absent; it exists at lowercase `installation.cpp`. Because `player_Get(i)` does **not** upper-bound `i` (`return g_player ? g_player[i] : nullptr`), `if(GetOwner() >= 0 && player_Get(GetOwner()))` was a genuine OOB read for `GetOwner() >= k_MAX_PLAYERS`; `Installation::RemoveAllReferences` now uses `safe_player(GetOwner())` at both sites. Lesson (again): verify a file's existence/case before annotating "absent."

**MISSING_BOUNDS_CHECK third mini-batch (2026-07-02).** `ProfileDB::DefaultSettings` validates/clamps `m_civIndex` (defaults to 16) against `g_theCivilisationDB->NumRecords()` before `Get()`; `victorymoviewin` guards a possible `-1` from `FindTypeIndex` before `GetMovieFilename`. `profileDB:549` whitespace-trim was already `len > 0`-guarded (stale). The remaining unverified bounds leads are concentrated in the deferred `net_*`/`network.cpp` packet flows and the slic-debug UI `k_AUI_LDL_MAXBLOCK` string buffers (`segmentlist`/`sourcelist`/`watchlist`/`thronedb`/`robotcom`/`tracklen`/`iparser`) — a P4-flavored string-safety cluster for a later pass.

### P4 — Unsafe string APIs

`strcpy`/`strcat`/`sprintf` are the buffer-overflow class — higher real-world risk than the raw-`new` count.

**Status 2026-07-03: clean-conversion pool exhausted at `unsafe_string_api=57` (first-party). Effectively complete; the remaining 57 is dead/deferred/idiom-cascade.** The arc: the counter started at a libs-inclusive **1360**, of which **1268 (93%) were vendored anet/freetype** — noise hiding the real surface. The ratchet was **re-scoped to first-party** (dropped `**/libs/**`, see P6 snapshot), giving a true baseline of **92**, then **85 genuine conversions** across the phase drove it to **57**, every one with a *verified* destination bound (fixed-array `sizeof`, exact-alloc size, documented `k_*` contract, or source-bound capping only where all callers were confirmed large enough). One **real overflow bug** was caught and fixed along the way: `tech_MemMap::GetFileExtension` copied an unbounded file extension into `extension[8]`.

The remaining **57** is *not* a clean-conversion pool:
- **Dead code (~20)** — `AgreementData::ToString`/`Interpret` (11), `BuildQueue::Dump` (5), `victorywin_GetRankName`, `texttable::GetText*`, `scenarioeditor::GetLabel` (whose `snprintf(ptr, sizeof(ptr), …)` truncation bug never runs): no callers, so no bound can be derived — the honest fix is **deletion**, not conversion.
- **Deferred network (~22)** — `net/` (18) + `netfunc` offset-writes into caller buffers of unknown capacity (4, TODO phase-2).
- **`GetXName(MBCHAR*)` idiom (~3)** — `CivilisationData`/`Player`/`Civilisation` "fill caller's buffer"; threading a size cascades to 40+ call sites incl offset-callers (`finalText+strlen`) where source-bound capping is unsafe — off-limits.
- **Hard cascades (~12)** — `UnitSpriteGroup::GetImageFileName` (24-caller in/out), `ldl_data::GetFullName` (recursive, elusive header), `c3errors` (Windows `LocalAlloc`), `civ3_main`.

**Dead-code deletion done 2026-07-12 (Olek's go-ahead):** deleted `AgreementData::ToString`/`Interpret` (+ the uncalled `Agreement::ToString` handle wrapper), `BuildQueue::Dump`, `victorywin_GetRankName`, `TextTable::GetTextEntry`/`GetTextHeader` — ~320 lines removed, all verified zero-caller (grep -a for the ISO-8859 files). One list entry was **stale**: `ScenarioEditor::GetLabel` IS live (tiledmap.cpp cell labels) and carried a real `snprintf(ptr, sizeof(ptr))` bug truncating every start-loc label to 7 chars — fixed by threading `labelSize` through (header + 1 caller) and bounding the raw `sprintf` in the same function. `unsafe_string_api` **57 → 39**; the remaining 39 is deferred `net/` (~22) + the off-limits `GetXName` idiom / hard cascades (~17).

- [ ] Convert unsafe-string clusters on crash-prone paths (parsers, UI text, save/load) to `strlcpy`/`snprintf`/`std::string`, lowering the `unsafe_string_api` ratchet baseline per batch.

**Started 2026-07-02.** `ControlTabPanel::AppendBlockName` switched from exact-sized `sprintf` to `snprintf`; `profileDB` now uses bounded `strlcpy` for parsed profile strings; and `c3files_getfilelist` now uses `strlcpy` for fixed-size filename buffers, lowering the ratchet from 1415 to 1410. `c3mem.cpp` also fixed a stale `size_t`/`%ld` format mismatch (`%zu`) but did not affect the unsafe-string counter because it already used `snprintf`. `SlicSegment::GetDescription` now uses its `maxsize` parameter instead of `sizeof(pointer)`. `SlicBuiltin` leader/pronoun and `SlicEngine::AddResearchOnUnblank` report leads were already fixed with `strlcpy`.

**Batch pass 2026-07-03 (1410 → 1376, −34).** Convention confirmed: BSD `strlcpy`/`strlcat`/`snprintf` from `<string.h>` (no project wrapper; my earlier "`n()` helper" was an `rg -r` typo artifact). Fixed-array `strcat`/`sprintf` → bounded equivalents on live crash-prone paths: `EditQueue` queue save/load/delete filename buffers (−12); `DiplomacyDetails` `interp[20000]`, `thronecontrol` `s[_MAX_PATH]`, `diplomacywindow` `finalText[k_MAX_NAME_LEN]` (4 of 8), `controlpanelwindow` `order` (−17); `cpw_NumberToCommas` threaded a `size_t size` param + `snprintf`, callers pass `sizeof(buf)` (−5). **Deferred (needs size threaded through multi-param signatures):** `diplomacywindow::GetProposalSummary`/`GetProposalDetails` (4 sites — callers pass `finalText + strlen(finalText)`).

**Ratchet re-scope (2026-07-03).** After finding the counter was 93% vendored `libs/`, dropped `**/libs/**` from `EXCLUDE_GLOBS`; libs-inclusive 1360 → first-party 92. See the P6 snapshot note.

**First-party grind 2026-07-03 (92 → 57, −35 real conversions).** All with verified bounds: netshell lobby UI (`allinonewindow` moreinfo/temp + scenario-name copies, `ns_customlistbox`, `gameselectwindow`, `netfunc::StringDup`, 17); `chatbox`/`dipwizard`/`aui_bitmapfont` (12, incl `ColorizeString` size-threading and a `memcpy` for the ellipsis writes); `aui_ldl`/`ear_util` exact/fixed buffers (4); `trademanager`/`loadsavescreen`/`textutils`/`effectspritegroup`/`civapp`/`aui_stringtable`/`battle.h` (8); `UIUtils::BlockPush` (2); `infowin_GetWonderCityName` + `BuildDefaultSaveMapName` threading (3); **`tech_MemMap::GetFileExtension` real `extension[8]` overflow fix** + `BuildDefaultSaveName` + `unitutil_GetCityInfo` (3); `victorywin_GetWonderFilename` threading (1); `c3files` literal assignment (1). Then the clean pool ran out — see the Status block above.

### P5 — JSON-load derived-cache audit

Known gap: JSON load skips some derived-cache rebuilds (`SetAllMoveCost`, `CalcChokePoints` unaudited) — wrong-but-not-crashing state after load.

- [x] Audit the load-path derived-cache rebuilds and pin them with a scenario fixture (`test/` slices are the net). **Done 2026-07-10.**

**Audit result (2026-07-10): the two named suspects need NO rebuild — their outputs are dense per-cell fields that round-trip.** JSON is the only save format now (`GameFile::RestoreGame` → `DispatchRestore` → `json_save::LoadJson`; the binary `World::Serialize` is gone). Findings per world derived-cache:

- **`SetAllMoveCost`** (`wldgen.cpp:1860`) → writes `Cell::m_move_cost` (via `CalcTerrainMoveCost`) + the movement-type bits of `Cell::m_env` (via `CalcMovementType`). Both are **dense-serialized** in the `Cell` bridge (`move_cost`, `env`) and restored verbatim, so no recompute is needed on load. Recomputing would in fact be **riskier**: `CalcTerrainMoveCost` depends on the cell's terrain-improvement objects (`m_objects`, not carried by the Cell bridge), which aren't necessarily re-linked at that point — serialising the final value is the correct choice.
- **`CalcChokePoints`** (`WrldCont.cpp:637`) → `SaveGF` writes `Cell::m_gf` (0/1 choke flag). `m_gf` is **dense-serialized** (`gf`) and restored verbatim → no recompute needed.
- **`NumberContinents`** → renumber not needed (`m_continent_number` is serialized per-cell, plus `continents_are_numbered` / `land_continent_max` / `water_continent_max`). Its **non-serialized** side-effect arrays (`m_land_size`/`m_water_size` and the `m_land_next_too_water`/`m_water_next_too_land` neighbor arrays, all `DynamicArray`/pointer state zeroed by `AllocateMap`) **are** the only world caches that genuinely need rebuilding — and `World::from_json` already calls `FindContinentSize()` + `FindContinentNeighbors()` (added earlier after ASan/SIGSEGV crashes in `MapAnalysis::BeginTurn` / the AI transport-goal path). Pinned by `scenario-city-capture` (meson.build:1730 — "the continent-cache … load rebuilds") and `scenario-load-stress` (load + 15 turns).
- **Capitol-distance cache** — `Happy::m_cost_to_capitol` / `m_dist_to_capitol` are **serialized** (Happy bridge); the lazy `m_capitolDistanceDirtyFlags` gate self-heals on the next map mutation (`Cell::CalcTerrainMoveCost` / city add-remove set it dirty). No gap.

**One real gap found + fixed (code):** the `good_value` (per-resource weighting from `ComputeGoodsValues`) load branch restored the table verbatim only when the saved length matched the current `ResourceDB`; on a mismatch (a mod changed the DB between save and load) it **left `m_goodValue` at whatever the throwaway fresh-game gameinit computed for the discarded initial map** — exactly the wrong-but-not-crashing state P5 targets. `World::from_json` now calls `w.ComputeGoodsValues()` in the mismatch/empty branch, rebuilding from the freshly-loaded cells (mirrors the map-gen + old binary path). Pinned by a new regression test (`test_json_save_integration.cpp` — "LoadJson derived cache (P5)…"): corrupt a save's `good_value` length, load it under a *different* seed, and assert the re-saved table equals the **source** map's values (proves the recompute reads the loaded cells, not the fresh-game map). Verified the test fails without the fix.

**Nets:** `LoadJson round-trip` (world byte-identity: `move_cost`/`gf`/`continent_number` all round-trip), the new `good_value` recompute test, `scenario-load-stress`, and `scenario-city-capture`. `make test` + `make ubsan-smoke` + integration + scenario suites green. Ratchet `raw_new` baseline 4818→4819 (a `--new-game` CLI-flag string literal in the new test trips the `\bnew\b` counter — a string false positive, consistent with the 10 such literals already absorbed in that file).

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

> **Ratchet re-scoped to first-party 2026-07-03.** `EXCLUDE_GLOBS` now drops `**/libs/**` (vendored anet/freetype/zlib/tiff/miles/GameWatch — upstream-owned, never a modernization target), the same rationale as the existing `build-*` exclusion. This affects **every** counter — the enforced first-party-only baselines became `raw_new=4819`, `raw_delete=1865`, `c_allocation=101`, `type_erased_casting=2644`, `unsafe_string_api=92` (down from the libs-inclusive 5472 / 2012 / 515 / 3739 / 1360). (`unsafe_string_api` has since been driven to **57** by the P4 grind.) The `Phase start`/`Removed` figures above were libs-inclusive; the first-party RAII work they count is unchanged. Trivially reversible (revert the one-line glob + baseline) if libs should be tracked again.

Files ticked: **17 / 574**; first-party baselines now **`raw_new=4799`, `raw_delete=1845`** after the 2026-07-11 session (9 conversions: CivilisationPool/TurnCnt/TradePool member `unique_ptr`s, MovePath transfer-on-success, Vision both members, settlemap + CalcChokePoints scratch `vector`s, and the `CombatField::m_field` `vector<vector>` conversion that fixed a real `new[]`/scalar-`delete` UB). (Ratchet runs ahead of ticked files because most touched files still have a harder residual cluster — e.g. `Sprite.cpp`, the sprite-group family. `ControlTabPanel.cpp` fixes a real `new[]`/scalar-delete mismatch but keeps one explicit `new[]` inside `unique_ptr<MBCHAR[]>`, so raw `new` does not drop; `scoretab.cpp` removed one raw `new`/`delete` pair.)

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

- [x] Write a "Remaining Legacy-Risk Areas" section in this file consolidating: leftover warning categories, ratchet counts, deferred systems (network, movies, Windows project files), and M7/M8 caveats. **Done 2026-07-10 — see below.**
- [x] Final verification pass (`make test`, `make ubsan-smoke`, clean worktree); update Overall Progress and declare the original Definition of Done met. **Done 2026-07-10:** `make test` (fast+unit+ratchet), `make ubsan-smoke`, and the integration + scenario suites all green on a clean worktree at commit `41860954`. **The original Definition of Done (M1–M8) is formally met.**

#### Remaining Legacy-Risk Areas (close-out consolidation, 2026-07-10)

The original DoD (M1–M8) is complete. What follows is the honest inventory of legacy risk the engine still carries — none of it blocks the DoD; it is the standing backlog the separate memory-safety phase (P2–P6) and future opportunistic work draw from.

**Modernization ratchet (first-party, enforced by `make test`).** Snapshot at close-out — these are floors, not targets; they can only ratchet down:

| Counter | Count (2026-07-17) | Nature of the remainder |
| --- | --: | --- |
| `raw_new` | 4802 | RAII conversion (P6); ~413 medium files need per-file ownership analysis, no mechanical tier. (Includes ~11 `--new-game`-style string false positives absorbed in test files.) |
| `raw_delete` | 1845 | same P6 effort (paired with `raw_new`). |
| `c_allocation` | 81 | `malloc`/`calloc`/`realloc`/`free`; 2026-07-17 loop drove 101→81 (RadarMap/ProjectFile → `std::vector`, `TifBuffer` RAII for TIFF loaders, dead code deleted). Remainder: slic `PSlicObject` ownership web (slicif/SlicSegment/SlicSymTab/json_save — one coherent mini-project), prjfile `getData_*` API, tileutils' `LowPassFilter` ownership-juggling corner (audit, not wrap). |
| `type_erased_casting` | 2581 | `void*`/C-cast handoffs; opportunistic cleanup only — large, cross-cutting, low crash-relevance. |
| `unsafe_string_api` | 17 | 2026-07-17 loop drove 39→17 (bounded appends, `net_order` dir-tag helper, size threaded through `GetImageFileName` — which also fixed a real `sizeof(ptr)` truncation bug). Remainder: `TODO(phase-2)`-annotated capacity-unknowable sites, the off-limits `GetXName`/`finalText` caller-buffer idiom, WIN32-only `c3errors`. |

**Sanitizer coverage gap (P2).** ASan is impossible natively on this macOS 26 (Tahoe) machine — a runtime re-entrancy deadlock in the ASan runtime that hangs even a trivial program (root cause pinned in the M1 caveat). `make ubsan-smoke` is the working sanitizer tier (shift/overflow/UB classes). The use-after-free / heap-overflow / leak classes ASan alone catches are **unguarded** until the Linux ASan tier (P2) lands — most valuable right before P6's RAII volume ramps, to catch conversion-introduced double-frees.

**Crash-class tail (P3/P4).** The HIGH bug-hunt categories are burned down, but a deliberately-deferred tail remains: network packet-size / null-deref flows (`net_*`, `network.cpp`, `slicif.cpp`), the supervised UI/geometry clusters (`DiplomaticRequestData`, blitter geometry, `WrlEnv` coordinate-contract methods), and the P4 dead/net/idiom string remainder. All are documented in the P3/P4 resolution blocks and the `BUG_HUNT_REPORT.md` annotations. `BUG_HUNT_REPORT.md` findings are **leads, not verdicts** — verify by code pattern, not the drifted May-2026 line numbers.

**Deferred subsystems (never a refactoring target without an explicit decision).**
- **Networking** (`net/**`, `ui/netshell/**`) — retained per M7; multiplayer resolve-later. Excluded from P3/P4/P6.
- **Movies** — wonder/victory movie DB/schema/data retained; the DirectShow references (`ui/aui_directx/aui_directmovie.*`, `aui_directmoviemanager.*` + their header closure `aui_directui.h`/`aui_directsurface.h`/`aui_directx.h`) are kept for a future SDL-based movie repair and are **never compiled on SDL builds**.
- **Windows project files** — legacy `.dsp`/`.mak` use DOS CRLF; `git diff --check` flags the pre-existing CR byte on changed lines — do **not** "fix" it (stripping the CR corrupts the VS6 format). These are not the canonical build (Meson is); Windows is best-effort and must not drive refactoring decisions.
- **Generated DB code** (`gs/newdb`, `gs/dbgen`) — fix at the generator, not the output.
- **Vendored libs** (`libs/**`: anet, freetype, tiff, zlib, miles, GameWatch) — upstream-owned; excluded from every ratchet since 2026-07-03.

**Leftover warning categories (M2; post-DoD, opportunistic only).** Include-case stragglers outside the swept paths; writable-string-literal conversions (`char *`/`MBCHAR *`) in diplomacy/UI/logging; unused-variable warnings in `governor.cpp`/`ArmyData.cpp`/`diplomat.cpp` (initializers may have side effects — review before removal); `&&`/`||` precedence in `ArmyData.cpp`/`robotastar2.cpp`; switch-exhaustiveness (e.g. `QuadTree.h` `QUADRANT_ERROR`) and overloaded-virtual in UI/sprite/network classes.

**Asset licensing (M8).** Game data is not part of the Activision/Apolyton source release — it is user-supplied and non-redistributable. Generated modern assets are local, rebuildable cache under `~/.ctp2/assets/<fingerprint>/`, never committed; original data stays canonical.

**Infrastructure caveat.** The local CI daemon (`.ci/daemon.sh`) is **not running** and `.ci/state.json` is stale — restart it before trusting `jq '.tier_a.status' .ci/state.json`, or agents read a month-old green.

### P8 — Performance build tier (release + LTO evaluation)

**Analysis 2026-07-12 (verified, measured).** The engine is C++20 and arm64-native, but **every existing build dir (`build`, `build-ubsan`, `build-sanitized`, `build-asan`, `build-cov`, `build-string-safety`) is `-O0 debug` with `hardening_level=maximum`** (`-ftrapv`, `_GLIBCXX_ASSERTIONS`, `-Wformat=2`). No LTO, no PGO, anywhere. The game has never been run optimized on this machine.

Measured (M4 Pro, 60 turns / 6 players / seed 42, headless):

| Tier | Wall | CPU | Outcome |
| --- | --: | --: | --- |
| `build` (debug, -O0, ftrapv) | 18.7 s | 18.2 s | baseline |
| `build-release` (release, -O2, hardening basic) | **3.5 s** | **2.1 s** | **identical metrics** (same seed → same scores/cities: deterministic across opt levels) |

That is **5.3× wall / 8.6× CPU** for zero code changes. The 2026-07-10 long-game findings ("per-turn cost grows superlinearly", 500 rounds = 6.5 min) were `-O0` artifacts; at `-O2` the same soak should run ~1 min.

**⚠️ `debugoptimized` trap:** `meson.build` gates `_DEBUG`/`SHOW_ASSERTS`/`USE_LOGGING` (and debug `-ftrapv`) on `buildtype.startswith('debug')` — which **matches `debugoptimized`**. A perf tier must use `buildtype=release` (as `build-release` does: `meson setup build-release ctp2_code -Dbuildtype=release -Dhardening_level=basic`), or that condition needs fixing first.

- [x] Make the release tier official. **Done 2026-07-12:** `make setup-release` / `make release` (buildtype=release, hardening basic, **thin-LTO**) + `make release-check` (the 300-round long-game soak against the optimized binary, to run per milestone). Measurements on M4 Pro:
  - 60 turns / 6 players / seed 42: debug 18.7 s wall → release-O2 **2.4 s wall / 2.1 s CPU**; thin-LTO **1.95 s CPU** (~7% over plain -O2 — enabled, since the tier is rebuilt occasionally, not per-edit).
  - 300-round soak: debug 71 s → release **16.8 s**, byte-identical outcome (round 300, 5/6 live, 42 cities — same as the debug run; optimization does not perturb determinism).
  - `-mcpu=native` not exposed (thin-LTO already in; revisit only if profiling on this tier shows a hotspot). The hardened `-O0` tier stays the dev/test default — the perf tier is additive.

**Threading non-goal (recorded so it isn't re-proposed):** the sim is single-threaded by design (threads exist only in `net/` and the smoke server); with 175 file-scope `g_*` singleton definitions and determinism guarantees (same-seed replays, save round-trips), multithreading the simulation is high-risk/low-need — release-tier `-O2` already buys 5–8×. Profile on the release tier before any concurrency discussion.

### P9 — Container modernization (legacy containers → std::)

First-party non-test usage counts (2026-07-12): **`PointerList<>` 554, `DynamicArray<>` 340, `SimpleDynamicArray<>` 60** — vs 1245 `std::vector` already in newer code. These custom containers predate the STL era of this codebase and are the locality/iterator-safety layer *next to* P6's raw-pointer work.

- [x] Add per-container ratchet counters. **Done 2026-07-12:** `pointerlist_uses=576`, `dynarray_uses=353` (DynamicArray + SimpleDynamicArray) enforced by `make test` alongside the other five.
- [ ] Burn down leaf/value-semantics uses to `std::vector`/`std::list`/`std::deque` per-cluster, smallest first — same discipline as P6 (no free mechanical tier assumed; serialization-coupled uses wait for their bridge; PointerList conversions are simultaneously P6 ownership decisions).

Caveats: `DynamicArray` has game-semantic quirks (POD `memcpy` growth, `Num()`/`Access()` idioms, ID-type coupling); `PointerList` is often *owning* — converting one is simultaneously a P6 ownership decision. A* pathfinding already uses an `AVLHeap` arena (`g_astar_mem`) — **leave it** (correct and hot).

### P10 — Type-erased casting burn-down

Ratchet `type_erased_casting = 2487` first-party (pattern: `void*` | `reinterpret_cast`). Top hotspots before cleanup: `gfx/spritesys/spritefile.cpp` (138 — binary sprite parsing), `ui/interface/controlpanelwindow.cpp` (132), `scenarioeditor.cpp` (99) — mostly the aui `void*` cookie-callback protocol.

**First cleanup 2026-07-13:** removed 42 redundant `(void *)` casts from file-I/O calls in `BaseTile.cpp` and `tileutils.cpp`; `fread`/`fwrite`/`c3files_fread` already accept typed pointers via `void*`/`const void*`, so this reduces ratchet noise without touching the by-design type-erased systems.

**Second cleanup 2026-07-14:** removed 20 more redundant `c3files_fread((void *)...)` casts from `tileset.cpp`; same no-logic-change file-I/O edge cleanup, lowering the ratchet to 2582.

**Third cleanup 2026-07-14:** removed 9 redundant `fread`/`fwrite`/`c3files_fwrite` casts from `spritefile.cpp`, `primitives.cpp`, and `ctpdb.cpp`; still only file-I/O edge cleanup, lowering the ratchet to 2573.

**Fourth cleanup 2026-07-14:** removed 86 redundant `SpriteFile::ReadData((void *)&...)` call-site casts in `spritefile.cpp`; the API still takes `void *`, but typed pointers convert implicitly, lowering the ratchet to 2487 without changing the binary read path.

- [ ] Opportunistic burn-down at the edges: replace C casts with checked casts where types are locally known; do **not** redesign the two by-design type-erased systems — the `GameEventArgument` GEA varargs event API (the `-Wno-non-pod-varargs` suppression exists for it) and the aui callback-cookie protocol. Lowest crash-relevance of the open items (UBSan tier + P1/P3 already covered the crash classes); slot behind P6/P9.

### P11 — GPU-path rendering

**Re-scoped 2026-07-12 (exploration overturned the "parked" framing):** the GPU present **already existed** — the SDL2 build presents via an accelerated `SDL_Renderer` (Metal on macOS, vsync, HiDPI `SDL_RenderSetLogicalSize`) + streaming ARGB8888 texture in `aui_SDLSurface::Flip()`. The engine composites in software (world → 16-bit `secondary`, UI dirty-rects → 32-bit `primary`), then one texture upload + present per frame.

**Stage 1 done 2026-07-12** (commits `2102b82d` + `5a463ac3`):
- **Pixel oracle first** — the present path was test-blind (no test read back a GPU frame; a black screen would pass everything). New smoke command `screenshot_presented <pres> [prim]` reads the screen texture back through a target texture (1:1, HiDPI-proof) and captures the software primary **atomically in the same dispatch**; `slice-ui` now asserts a 7×7 sample grid identical between primary and GPU readback + non-monochrome.
- **Dirty-rect present** — `aui_UI` accumulates the union of every rect composited into the secondary (choke points `BltToSecondary`/`ColorBltToSecondary`); the two per-frame mouse presents opt in via `BltSecondaryToPrimary(flags, useAccumulatedDirty)`, scoping both the 565→8888 convert and the `SDL_UpdateTexture` upload (`Flip(RECT const*)`) to it. Direct `Secondary()` writers (splash, movies) keep full-frame defaults; empty union → full frame. Verified: TiledMap/world content reaches the secondary via window surfaces + tracked dirty lists, so the union covers the true frame delta.
- **Measured** (debug tier, 1024×768, idle + turns): average converted/uploaded area **100% → 26.7%** of the frame (−73% bytes); per-present wall ~unchanged (vsync-block dominated) — the win is CPU work/battery inside the frame budget, and it scales with resolution.

**Stage 2 un-parked 2026-07-14 (reprioritized ahead of P2/P6 memory-safety work by Olek).** Goal: real GPU compositing + draw-time render flags on the Stage 1 dirty-union + pixel-oracle foundation. Ordered checklist below; `(~Ns)` = rough session estimate once unblocked.

- [x] **A. 32bpp UI blitters + screen surfaces** — DONE 2026-07-14 (commits `e85df21d`…`33701943`). Primary+secondary are ARGB8888; `ColorBlt`/chroma-key setup plus general same-format `Blt`/`TileBlt`/`BevelBlt`/`ColorStencilBlt`/`StretchBlt`/`SpanBlt` handle the common 32bpp UI paths — the present path no longer converts 565→8888 for screen-surface clears/fills/transparency/UI blits. Covered by focused doctests; verified with `make test`, SDL3 `fast unit`, and sequential `slice-headless`/`slice-ui` (incl. the presented-frame pixel oracle). Direct `Secondary()` writer audit clean (splash routes through primitives/blitters; raw movie copies are DirectX-only; the debug font-cache dump self-rejects non-16-bit surfaces).
- [x] **B. World render pipeline → 32-bit — DONE + VERIFIED 2026-07-14.** The world surface is 32-bit ARGB8888 and the `slice-ui` pixel oracle confirms the presented frame across multi-turn gameplay (from the canonical repo-root `build/`; `ctp2_code/build` gives the wrong cwd and fails at ProfileDB). B1 modern-first atlas path also landed: PNG loader (zlib) → `ModernSpriteAtlas` (load + `Blit`) → `~/.ctp2/assets/current` pointer → modern-first hook in `UnitSpriteGroup` (env `CTP2_MODERN_SPRITES`, default off). Verified: with the toggle on, `slice-ui` PASSES end-to-end (units drawn from the atlas). Remaining polish: draw-flags (fog/desat/transparency) + scaled/mirrored facings on the atlas path; Good/Effect group draw swap; C (GPU fog-mask). Original detail below.
- [~] **B. (historical) World render pipeline → 32-bit** — was IN PROGRESS 2026-07-14 (not asset-blocked). Two premises here were wrong: (a) `k_SHARED_SURFACE_BPP` is **dead code** (referenced nowhere) — the real depth knob is a hardcoded `16` at `ctp/civ3_main.cpp:438` → `aui_UI::m_bpp`; (b) B does **not** need the atlas first — expanding 565→8888 *at the tile blitter's store* is the same per-pixel cost as today's write (no extra pass), so the world goes 32-bit via **expand-at-blit** and the atlas (B1) is a separate source-fidelity track. Byte-identity strategy: keep per-pixel math in 565, expand only at the store (`pixelutils_StorePixel`) — tiles round-trip byte-identically; dest-reading ops use true-8888 (`pixelutils_*8888`). Progress on branch `p11-stage2-gpu-world`:
  - [x] B2a. `pixelutils_565to8888`/`_16to8888` expanders + `pixelutils_StorePixel` + true-8888 dest ops, doctested; bpp-aware `aui_Factory::new_Surface` (P11 B2.1).
  - [x] B2b. **All ~36 raw-pixel world tile writers converted** to the dormant 32-bit path (6 by hand + 28 via a 4-way subagent fan-out + `PaintColoredTile` + a bpp-agnostic `ScrollPixels` rewrite).
  - [ ] B2c. Sprite blitters (`gfx/spritesys/spritelow.cpp`, 9 funcs) + `ui/aui_utils/primitives.cpp`.
  - [ ] B3. Flip `m_mapSurface` to 32-bit (`tiledmap.cpp:299` → `new_Surface(...,32)`); the implicit `SDL_BlitSurface` 565→8888 convert then self-neutralizes. Re-assert the `slice-ui` pixel oracle + a golden byte-identity check. (Pan-test too: `ScrollPixels` isn't oracle-covered.)
- [x] **C. GPU fog-mask on the world texture** — DONE + VERIFIED 2026-07-15 (commits `7a5eeade`/`e19e9237`/`73b3750e`), gated on env `CTP2_GPU_FOG` (implies `CTP2_GPU_LAYERS`), default OFF. Terrain fog moved off the CPU per-tile blend onto a GPU-composited mask: the CPU pass renders terrain UNFOGGED (`TiledMap::GpuFogActive()` forces `fog=false` at all 5 terrain fog sites), `TiledMap::BuildFogMask` rasterizes fogged-tile diamonds (50% black) into a screen-space mask surface reusing the terrain tile geometry, and `aui_SDLSurface::Flip` composites it (BLENDMODE_BLEND) between the world and UI copies so it darkens ONLY the world. **Verified in-game by Olek** (fogged terrain darkens ~50% aligned to tiles, unexplored black, UI stays bright); the `slice-ui` scenario has no fogged tiles so the auto-oracle only proves no-regression. Desaturation stays CPU (per-sprite granularity). Follow-up: mask scaling at non-default zoom. Engine has NO SDL_gpu/shader path — this is the SDL_Render 2D mask approach, not a fragment shader.
- [x] **D. Per-layer `SDL_Texture` GPU compositing** — DONE + VERIFIED 2026-07-15 (commits `27aee5a8`/`a73a535e`/`10dbac6b`/`bc5e0ae4`), gated on env `CTP2_GPU_LAYERS` (default OFF). Splits the frame into a **world-only** GPU texture (terrain+units+background) and a **UI-only** alpha texture, GPU-composited (world base + UI in BLENDMODE_BLEND) instead of the single merged screen texture. Not the full per-window aui rewrite — a minimal world/UI split (exactly what C + F need): the `aui_UI::BltToSecondary`/`ColorBltToSecondary` chokepoint mirrors each composite write into `m_worldSurface` (bg window) or `m_uiSurface` (everything else, transparent base); `aui_SDLSurface::Flip` uploads both and composites; the `slice-ui` oracle re-composites the same two layers so it stays exact. **slice-ui green both default and `CTP2_GPU_LAYERS=1`.** Known limitation (documented, static-frame-unaffected, dynamic follow-up): UI layer not transparency-vacated when a UI window moves over the world (would ghost) — fix is a transparent region-clear on world redraw.
- [x] **E. Phase D — SDL3 as the default backend** — DONE 2026-07-14. Full integration suite green under SDL3 (all 6: in-binary `integration` doctest, `slice-headless`, `slice-ui` pixel oracle, research/explore/expansion multi-turn AI). `sdl_backend` default flipped `sdl2`→`sdl3` (fresh no-arg `meson setup` picks sdl3; main `build` reconfigured); `Brewfile` → sdl3/sdl3_mixer. **SDL2 stays fully supported** via `-Dsdl_backend=sdl2` (still configures + builds). `make test` + `slice-ui` green on the sdl3-default build. **En route, found + fixed a GPU-present regression dead since `b0d7a227`:** the present moved to `m_secondary->Flip()` but the secondary was created `isPrimary=FALSE`, and `aui_SDLSurface::Flip()` only uploads to the screen texture + presents when `m_isPrimary` — so the present was a no-op and the presented frame was solid black on **both** backends (commit `028f3606`). It hid because the P11 verify loop ran `meson test integration` (the *named* in-binary doctest), which excludes the `slice-ui` subprocess pixel oracle — the one test that reads a real presented frame back. **Lesson: verify P11/present changes with `meson test --suite integration` (or the explicit `slice-ui` test), not `meson test integration`.**
- [x] **F. Smooth camera** (momentum scroll, pinch-zoom) — DONE 2026-07-15/16 (commits `665a90e8`/`0d41fa24`/`fd090aec`/`9c82926e`), gated on env `CTP2_GPU_CAMERA`. Mouse-wheel / trackpad drives a GPU camera transform (world texture scaled/panned at present time); momentum physics with spring-back zoom. En route: fixed the mouse path eating wheel events so they reach the camera.

**Trackpad panning — SHIPPING (default-on), active 2026-07-16.** Distinct from the env-gated GPU-camera-F above: two-finger trackpad scroll (and the physical wheel) now pan the map via the real engine `ScrollMap` — reveals terrain, no GPU-camera/layers dependency, on by default.
- [x] **Core tile-stepped pan** — DONE + verified in-game by Olek 2026-07-16 ("works very nice"). `ui_HandleTrackpadPan` (`civ3_main.cpp`) accumulates `SDL_MOUSEWHEEL` x/y (SDL3 fractional deltas) as map pixels and emits a whole-tile `ScrollMap` step per tile crossed (`hscroll`=tile width, `vscroll`=half tile-row — same units as edge-scroll), reusing `ui_HandleMouseWheel`'s Refresh sequence; residual dropped at map edges. Zoom moved off the wheel onto the (not-yet-wired) pinch gesture. macOS natural-scroll direction, tuned `k_PAN_PIXELS_PER_WHEEL=24`.
- [x] **Sub-tile smoothness — buttery GPU pan (ADR-001, shipped via ADR-002).** Decided 2026-07-16: oversized GPU world texture (screen + margin) panned on the GPU via a moving source rect; whole-tile `ScrollMap` recenters underneath, margin hides the seam; macOS supplies native momentum via the wheel-event stream. **Parked same day** (glide never visibly smooth), then **root-caused and SHIPPED 2026-07-16/17** — see the wrap-up block after 2d. Historical incremental notes (2a/2b implementation superseded by ADR-002's window mirror):
  - [x] **2a. Oversized world substrate** — DONE 2026-07-16. World surface + texture oversized by `aui_SDL::WorldMargin()` (96px) each side (streaming layer path; parked quad path stays screen-sized); the D mirror centres the background-window write in the oversized surface (`aui_ui.h`, offset = half the size diff); `Flip` presents a screen-sized viewport **windowed** into the oversized world texture via a source rect (`CTP2_SDL_RenderTextureWindow`, float src for sub-pixel on SDL3), slid by `CameraOff`/scaled by `CameraZoom` — identity at (0,0,1) reproduces the pre-margin frame. The `screenshot_presented` oracle updated to mirror the windowed present (target sized to the SCREEN texture, world/fog windowed). Verified: slice-ui green in default, `CTP2_GPU_LAYERS=1`, and `+CTP2_GPU_CAMERA=1`; `make test` green. Margin border is unshown until 2b fills it.
  - [x] **2b. Render the margin (with actors)** — DONE 2026-07-16. `TiledMap::RenderWorldLayer` renders terrain (`RepaintTiles`) **and unit/city actors** (`RepaintSprites`) directly into the oversized `m_worldSurface` at a temporarily-widened view (save/restore, so gameplay/mouse coords are untouched), publishing the whole-tile pixel content offset via `aui_SDL::SetWorldContentOffset`. Called from `background_draw_handler` when GPU layers are on; replaces the old screen-mirror of the world layer (the `aui_UI::BltToSecondary` world branch is now disabled — UI still mirrors). The present + oracle window the screen viewport at the published offset. This is option B's *result* (actors in the margin, real render) via a safe direct pass — **no** oversizing of the aui background window / no coordinate-system rewrite. slice-ui green default + `CTP2_GPU_LAYERS=1` (center matches the CPU frame → render + offset correct); `make test` green. (Trade-route lines in the margin deferred — thin, `tradepool` not imported in tiledmap.)
  - [x] **2c. Wire the pan** — DONE 2026-07-16. `ui_HandleTrackpadPan` now feeds whole tiles to `ScrollMap` (reveals terrain+actors) and the sub-tile remainder to `aui_SDL::SetCamera` (GPU offset), presenting immediately so the glide shows between tile steps. `MousePointToTilePos` shifts picks by `CameraOff` so clicks match the visibly-shifted map. Ignored unless the GPU camera is on, so the default build stays tile-stepped. slice-ui green default + `CTP2_GPU_LAYERS=1 CTP2_GPU_CAMERA=1` (no regression; live buttery pan needs in-game trackpad verification).
  - [x] **2d. Ship** — DONE 2026-07-17 (`2cebf01a`): `CTP2_GPU_LAYERS`+`CTP2_GPU_CAMERA` **default-on** (`=0` opts back to the legacy present); edge-of-screen + arrow-key scrolling routed through the same eased camera target as the trackpad (legacy `ScrollMap` stays as the camera-off fallback); full integration suite green under the new defaults.

**Buttery-pan wrap-up (2026-07-16/17) — why it was invisible, and what shipped.** The pixel proof (`pan-pixel-proof` meson integration test — forces a camera offset, asserts presented terrain pixels shift; plus a glide phase streaming pan pulses and asserting sub-tile frame positions) found **two stacked bugs** the parity oracles could never see: (1) the world GPU layer had been **empty since Stage 2 D** — `SetWorldWindowSurface` captured the background window's surface before its lazy creation, so the routing key was NULL and every blit fell to the UI layer; the camera panned an empty texture under an opaque UI carrying the whole frame. Fixed by routing on a live `WorldSurfaceKey()` (window stored, surface resolved per blit) + **hole-punching** stale UI-layer pixels under world writes (`c15db527`). (2) The parked `RenderWorldLayer` margin re-render placed terrain (+94,+72 = window margin) and sprites (cached screen coords) at **inconsistent offsets** — replaced by **ADR-002**: the legacy background window surface *is* an oversized world render (one tile-grid margin, terrain+actors via the battle-tested pipeline); mirror it 1:1 into the world layer at `BltToSecondary` (whole-surface — composite rects are screen-clipped and would leave stale margins), content offset pinned to the window margin (94,72, `static_assert`ed vs `k_TILE_GRID_*`), recenter forces ±1 tile since the 94px margin is smaller than the 96px tile step and redraws+composites synchronously (`ee4134b1`). Also en route: `ScrollPixels` OOB clamp cherry-picked (SIGSEGV fix, `f86fbc11`); edge-scroll suppressed under `--smoke-test` (mouse pinned at (0,0) read as permanent scroll-up-left — why every harness screenshot had a black map, `fed82438`); `camera_debug_set/center/pan/layers` TEMPORARY debug verbs.

- [x] **Hardware cursor in layered mode** — DONE 2026-07-17 (`f237a24a`). The software cursor is incompatible with the layered present: `ReactToInput` picks up SECONDARY pixels under the cursor and "restores" them through `BltToSecondary` into the persistent UI layer — stale screen-coord squares floating over the sliding world (seen as cursor strobing and "units in the wrong tile"). With layers on, the game cursor image (chroma-keyed) converts to an OS cursor (`SDL_CreateColorCursor`, synced on cursor change + animation), and every software pickup/mix/restore path is skipped. Zero-latency cursor as a bonus.
- [x] **Pinch-to-zoom** — DONE 2026-07-17 (`f6a354f9` + `feceecde`). SDL3/macOS forwards **no** trackpad touches by default (its `SDL_TRACKPAD_IS_TOUCH_ONLY` hint would break mouse control), so macOS pinch arrives via a native Cocoa `NSEventMaskMagnify` local monitor (`os/osx/osx_pinch_monitor.mm`, the project's first Objective-C++ file; consumed once per frame outside the pump) driving the engine `ZoomIn`/`ZoomOut` through the same guarded gate; ~0.30 magnification per step ≈ 2–3 steps per relaxed pinch. The SDL touch path (`PinchDetector`, pure geometry, unit-tested incl. two-finger-scroll rejection) stays for platforms that deliver finger events. En route: `-lltdl` became a real `find_library` dependency (the objcpp language switch broke the bare link flag).

**Stage 3 — GPU terrain quads (G).** Draw terrain as GPU textured quads from a tile atlas into the world render-target texture, instead of uploading the whole CPU-composited world surface each frame — so pan/zoom/fog (D/C/F) transform a complete, hole-free world texture. Gated on env `CTP2_GPU_QUADS` (implies `CTP2_GPU_LAYERS`), default OFF.

- [x] **G0. Tile cache + LRU atlas allocator** — DONE 2026-07-16 (commit `d7502f49`). `GpuTileCache` maps a `TerrainCellSignature` (tile num + tileset index + 4 transition edges, packed into `uint64`) → an atlas slot, with LRU eviction over a cols×rows grid. Covered by `test/cpp/test_gpu_tile_cache.cpp` (in `make test`).
- [x] **G1. Live quad renderer** — DONE + no-regression-verified 2026-07-16. `TiledMap::BuildTerrainQuads` (called from `Refresh` after the CPU passes unlock) mirrors `RepaintTiles`' visible-cell iteration: each explored cell's signature keys the cache; on a miss it composites the cell **once** via `DrawTransitionTile` into a 94×72 transparent scratch surface and uploads it to its atlas slot; hit or miss, it appends an atlas-src→world-dst `GpuQuad` to a per-frame draw list. `aui_SDLSurface::Flip` sets the world texture as the render target (created `TARGETACCESS` in quad mode vs `STREAMING` otherwise), clears to opaque black, and draws every quad from the atlas — a complete world texture the D/C/F composite then transforms. **slice-ui pixel oracle green in both default and `CTP2_GPU_QUADS=1` modes** (oracle compares CPU software primary vs GPU readback → the quad render lands at the same positions/colors as the CPU path); cache fills once and holds stable across frames. Only the singleton main map drives the (single, global) draw list; radar/thumbnail maps are guarded out. **Full-map visual fidelity still wants an in-game look** (the slice map has ~9 terrain cells, mostly fog) — as with C/D. Follow-ups: engine zoom levels (G1 is largest-zoom only), draw-flags parity, atlas overflow tuning.

**⚠️ G-series (terrain quads) PARKED 2026-07-16 — the camera goal it targeted is already met without it.** Investigation while scoping "G2 (units/cities on the quad path)" found two things: (1) plain layer mode (`CTP2_GPU_LAYERS=1 CTP2_GPU_CAMERA=1`, no quads) already uploads the **full** `m_worldSurface` into the world texture every frame → a complete, hole-free world that the F camera pans/zooms (verified: slice-ui green). The "dirty-rect holes" the quad path was meant to fix don't exist in the full-upload path. (2) Making quad mode *correct* needs **all** non-terrain map content (units, cities, roads, rivers, borders, grid, goods, effects — everything `CalculateWrap`/actors draw) split into a separate transparent layer, because it's all mixed into the one background surface; terrain quads reproduce only `DrawTransitionTile`. That's a large two-layer-render undertaking whose only payoff over full-upload layer mode is avoiding a ~3 MB/frame (33 MB at 4K) upload — trivial on Apple unified memory. **Decision (Olek):** stop the quad line; the smooth-camera / GPU-composite goal is DONE via layer mode. G0/G1 stay committed but gated OFF (harmless). The genuine home for an atlas/GPU-sprite path is the **modern-asset sprite track** (below), which Olek chose as the next direction — source fidelity (32-bit RGBA sprites), composited through the existing pipeline, no terrain/actor split needed.

**Modern-asset sprite track — active 2026-07-16 (not asset-gated: 463 `.SPR` on disk, full 463-atlas cache generated at `~/.ctp2/assets/current`).**

> **⚠️ Reach correction (2026-07-17): the modern hook is on `DrawDirect`, which the MAIN interactive map does NOT use.** `UnitActor::Draw` (the primary map render, via `PaintUnitActor`) calls `UnitSpriteGroup::Draw` — **no modern hook** → legacy. `UnitSpriteGroup::DrawDirect` (where the hook + all the mirrored/scaled/Good/Effect work lives) is reached only by `RenderPlayerView`/`RenderFullMap` (the `render_map*` screenshot/timelapse export), the **battle view**, and the **resource/work overlay** maps. So modern sprites currently render in those, but the primary gameplay map is still legacy. **The real enabler is wiring the modern atlas into the interactive `Draw` path** (`UnitSpriteGroup::Draw`, plus Good/Effect/City `Draw`). Subtlety: that path draws to the screenmanager's **pre-locked** surface (`RepaintSprites` → `screenmanager LockSurface`), so it needs a lock-free `Blit` variant writing to the already-locked base (the `WriteEffectPixel` helper is already factored for reuse). This supersedes the earlier "slice-ui shows units from the atlas" note — that was the screenshot/overlay path, not the main map.
- [x] **Draw-flags parity on the atlas path** — DONE 2026-07-16. `ModernSpriteAtlas::Blit` now honours the three mutually-exclusive per-pixel draw flags (`k_BIT_DRAWFLAGS_TRANSPARENCY`/`_FOGGED`/`_DESATURATED`) with the same precedence as the legacy RLE draw (`spritelow.cpp`): transparency alpha-blends, fog shadows, desaturate greys. Runs in **full 8888** straight from the atlas RGB on the 32-bit destination (`pixelutils_BlendFast8888`/`_Shadow8888`/`_Desaturate8888` — no 565 round-trip, so the modern path keeps its extra colour fidelity; the 16-bit branch uses the 565 helpers to match that surface). `UnitSpriteGroup::DrawDirect` threads `transparency`+`flags` into the atlas draw. Proven by a new doctest (`test_modern_sprite_atlas.cpp`) asserting each flag's blitted pixel equals the reference `pixelutils_*8888` result; `make test` + `slice-ui` (modern on) green. Outline/feathering remain legacy-only.
- [x] **Mirrored facings 5–8** on the atlas path — DONE 2026-07-17. `ModernSpriteAtlas::Blit` gained a `mirror` flag (samples source columns right-to-left); `UnitSpriteGroup::DrawDirect` folds facing ≥ `k_NUM_FACINGS` onto its stored counterpart (`k_MAX_FACINGS - facing`) and measures the reversed draw origin from the frame's right edge (`drawX - (w - hp.x)`), matching `FacedSprite::Draw`. Column-flip proven by a new `test_modern_sprite_atlas.cpp` doctest; `make test` + `slice-ui` (modern on) green. Reversed-facing *geometry* still wants an in-game look (the slice scenario doesn't force facings 5–8).
- [x] **Scaled facings** (zoom ≠ 1) — DONE 2026-07-17. `ModernSpriteAtlas::BlitScaled` nearest-neighbour scales the frame into a destW×destH box (shares the per-pixel effect write with `Blit` via an extracted `WriteEffectPixel` helper); Unit / Good / Effect draws now pass `scale` through and use it at zoom ≠ 1, scaling the hot-point origin like the legacy `DrawScaledLow`. New doctest covers upscale + mirrored scale + empty/unknown guards. Net −1 type-erased cast (both locks moved to a cast-free `LPVOID` + `static_cast` pattern; baseline lowered). **Known minor fidelity gap:** the smallest zoom nearest-neighbour downscales the full frame instead of using the legacy precomputed miniframes — refinable, and modern is opt-in.
- [~] **Interactive `Draw` path (main-map enabler)** — the real "use new sprites in play" step.
  - [x] **Units** — DONE 2026-07-17. `ModernSpriteAtlas` gained lock-free `BlitLocked`/`BlitScaledLocked` cores (a `LockAndRun` helper makes `Blit`/`BlitScaled` thin lock-then-delegate wrappers — no duplication); `UnitSpriteGroup::Draw` now composites the atlas frame into the ScreenManager's already-locked surface via a new `DrawModernInteractive` helper (same faced geometry as the DrawDirect hook: folded facings 5-7, right-edge reversed origin, scaled origin, `BlitLocked` vs `BlitScaledLocked`; skips directional attacks). Gated on `CTP2_MODERN_SPRITES`, so the default build is byte-identical. Verified: `make test` green; `slice-ui` green with modern **on** (interactive path exercised, no lock conflict/crash) and **off** (unchanged); `pan-pixel-proof` green.
  - [x] **Good / Effect** `Draw` — DONE 2026-07-17. Lock-free `ModernSpriteDrawUnfacedLocked` twin (surface `ModernSpriteDrawUnfaced` now locks + delegates to it via `LockAndRun`); `GoodSpriteGroup::Draw` (IDLE) and `EffectSpriteGroup::Draw` (PLAY; additive FLASH stays legacy) composite into the ScreenManager's locked surface. `make test` + `slice-ui` (modern on/off) green.
  - [x] **City** — no work needed: cities load as a **`UnitSpriteGroup`** (`SpriteGroupList.cpp:127`, `GROUPTYPE_CITY → new UnitSpriteGroup`) and render via `UnitSpriteGroup::Draw`, so they are already covered by the units wiring above. The `CitySpriteGroup` class is unused for city rendering.
- [x] **City group draw swap** — N/A (2026-07-17): cities are loaded as `UnitSpriteGroup` (`SpriteGroupList.cpp:127`), not `CitySpriteGroup`, so they draw via `UnitSpriteGroup::Draw` and are covered by the interactive-units wiring. The 110 GC atlases are used through that path.
- [x] **Outline / feathering** — DONE 2026-07-17. All modern hooks (Unit/Good/Effect, `Draw` + `DrawDirect`) now defer to legacy when `outlineColor != 0`, so any outlined draw keeps its silhouette. NB the concern was smaller than feared: the interactive `UnitActor::Draw` passes `outlineColor = 0x0000` (line 1352) — the main-map selection highlight is a separate `DrawSelectionBrackets` overlay, not a sprite outline — so the guard is belt-and-suspenders. `make test` + `slice-ui` (modern on/off) green.
- [x] **Good/Effect group draw swap** — DONE 2026-07-17. Two shared free helpers added to `ModernSpriteAtlas` (`ModernSpriteLoadIfEnabled` — replaces the Unit-local static loader — and `ModernSpriteDrawUnfaced`, which replicates the non-faced `Sprite::DrawDirect` hot-point + facing≥5 reversal and falls back to legacy on `k_BIT_DRAWFLAGS_ADDITIVE`). `GoodSpriteGroup` (IDLE) and `EffectSpriteGroup` (PLAY only; the additive FLASH overlay stays legacy) gained an `m_modernAtlas` member + out-of-line ctor/dtor. New doctest covers placement/mirror/additive-fallback; `make test` + `slice-ui` (modern on) green.
- [x] **Real v2/LZW1 `.SPR` parity** — DONE 2026-07-17. Exactly **one** v2/LZW1 file exists in the shipping data (`GG023.SPR`, a GOOD sprite; scan: 313 v0 / 149 v1 / 1 v2). Python side: `spr_export.py --verify GG023.SPR` decodes the **real** LZW1 stream and all 69 frame rows pass the per-row width invariant (structural parity on real data), on top of the existing synthetic self-test. Engine side (the real gap — `SpriteFile::DeCompressData_LZW1` had **zero** tests): new `test_spritefile_lzw1.cpp` doctest exercises the protected decoder via a test-access subclass on the same copy-mode + back-reference vectors the Python self-test uses, proving both decoders implement the identical documented algorithm (`SpriteFile.h:57-61`). `make test` green. Residual (low-risk, single file, not committed — asset can't be): a full engine-vs-Python byte diff of GG023's decoded frames; both decoders are independently verified on the algorithm and the engine loads GG023 in-game.

**Stage 2 tally (2026-07-17): A–F ALL DONE and DEFAULT-ON** — buttery pan, hardware cursor, pinch zoom ship on a stock launch (`CTP2_GPU_LAYERS=0` opts out). Quad path (G) parked (see above). Remaining P11 work: the modern-sprite track checklist above (mirrored/scaled facings, Good/Effect groups, real v2 parity) + known cosmetic gap: cosmetic zoom-out (`CameraZoom < 1`) can sample past the 94/72px mirror margin (spring-back peek only).

### M10 — Modern-asset converter + first-run (un-parked 2026-07-14)

Offline converter (packed atlas + JSON manifests into `~/.ctp2/assets/<fingerprint>/`) plus an engine modern-first loader with legacy fallback. See `docs/modern-assets.md`.

**Converter COMPLETE 2026-07-14 (Stage 0).** The "no licensed `.SPR` committed / needs Olek's data path" framing below is **obsolete** — the data is on disk at `ctp2_data/default/graphics/` (463 sprites). `spr_export.py` now decodes all 463 (GOOD/EFFECT added; the lone v2/LZW1 file `GG023` decodes via filename-prefix fallback — the engine loads by GROUPTYPE, not the junk type field); PNG chosen as the atlas format with a pinned decode contract; the full 463-atlas cache is generated + validated; a C++ doctest loads a real generated manifest. The engine loader (PNG decoder on vendored zlib + atlas RGBA draw + modern-first/legacy-fallback at `SpriteGroupList::LoadSprite`) is the remaining M10 work, done alongside P11 B1. NB: atlas-backed textures are **not** a hard prerequisite for the 32-bit world (B uses expand-at-blit); the atlas is a source-fidelity track.

**B1 started 2026-07-14:** `spr_export.py` has a pure Python LZW1 decoder wired into the v2 normal-frame parser, verified with synthetic copy-mode, compressed literal/back-reference streams, and a synthetic v2 frame payload because no licensed `.SPR` assets are committed. Real v2 parity remains the next B1 step and needs Olek's local data path.

**B2 started 2026-07-14:** `spr_export.py --atlas` now packs decoded v0/v1 unit frames into one dependency-free RGBA PNG atlas plus manifest rects using the existing decoder output. `--modern-assets` writes under `~/.ctp2/assets/<source-fingerprint>/`; when pointed at a directory it recursively walks `.SPR` files and fingerprints the source set from relative paths + contents. Both are verified by the synthetic self-test; real atlas parity still needs a local `.SPR` data path.

**B2 validator 2026-07-14:** `spr_export.py --validate-manifest <json>` checks atlas manifest self-consistency (atlas metadata, per-frame references, rect bounds). This is the loader-contract guard before any engine modern-first path trusts generated assets.

**Phase C seam started 2026-07-14:** the engine now has a small `ModernSpriteManifest` C++ parser/validator for generated atlas manifests, covered by fast doctests. It does not load textures or change rendering yet; it only establishes the validated data shape the modern-first loader will consume.

**Phase D SDL3 backend probe 2026-07-14:** local pkg-config has `sdl3=3.4.12` and `sdl3_mixer=3.2.4`. The `-Dsdl_backend=sdl3` Meson build now links `ctp2`, `ctp2_headless`, `ctp2_fast_tests`, and `ctp2_unit_tests`, with `meson test -C build-sdl3-probe fast unit` green. SDL2 remains the default backend.

### P12 — Single `~/.ctp2` data root (install script + engine relocation)

**Scoped 2026-07-17 (Olek).** Goal: **one executable that reads *all* game data from `~/.ctp2`**, so the ~476 MB in-tree `ctp2_data/` can be deleted from the working tree. Not a "convert everything to modern formats" project — a **relocation** that copies the canonical data out of the tree once, keeps it as the source of truth (`original_data/`), and converts the subset we *have* converters for (sprites) alongside it. The modern-format track (below) then lights up incrementally without ever blocking the relocation win.

**⚠️ Prerequisite — finish wiring modern sprites to the engine FIRST (Olek's explicit sequencing).** The modern-first atlas path already exists but is gated OFF and incomplete: `ModernSpriteAtlas::Load` reads `~/.ctp2/assets/current/<name>.json` (`gfx/spritesys/ModernSpriteAtlas.cpp`), hooked into `UnitSpriteGroup` behind `getenv("CTP2_MODERN_SPRITES")`. Before relocating, close the gaps from the modern-asset sprite track above so the engine genuinely *uses* `~/.ctp2` for sprites on a stock launch:
- [x] Mirrored facings 5–8 on the atlas path — DONE 2026-07-17 (see modern-asset sprite track above).
- [x] Scaled facings (zoom ≠ 1) — DONE 2026-07-17 (see modern-asset sprite track above; smallest-zoom miniframe fidelity is a documented minor gap).
- [x] Good/Effect group draw swap — DONE 2026-07-17 (see modern-asset sprite track above).
- [x] Real v2/LZW1 `.SPR` parity — DONE 2026-07-17 (one v2 file GG023; Python `--verify` passes on real data + new engine-decoder doctest; see modern-asset sprite track above).
- [ ] Decide default-on for `CTP2_MODERN_SPRITES` once the above hold (or keep opt-in with a documented reason).

**Target layout** (all under a configurable root — see env override):
```
~/.ctp2/
  original_data/   verbatim copy of ctp2_data/ — the engine's canonical source today
  assets/<fp>/     modern sprite atlases (already generated; loader gated on CTP2_MODERN_SPRITES)
  saves/           games, queues, mp, scen, maps, clips
```

**Work items (do AFTER the prerequisite):**
- [ ] **Install script `tools/assets/install_ctp2_home.py`** (run via `mise exec -- python …`):
  1. Mirror-copy `ctp2_data/` → `$CTP2_HOME/original_data/` verbatim (idempotent; skips unchanged files). Copy **all** data first — the engine runs off `original_data/` regardless of conversion state.
  2. Invoke the existing `spr_export.py --atlas --modern-assets` to (re)generate `$CTP2_HOME/assets/<fingerprint>/` from the copied originals.
  3. `--purge` (opt-in, OFF by default): delete the in-tree `ctp2_data/` **only after** a verified copy (file-count + size check); refuse on mismatch.
  4. `--dest <dir>` / `CTP2_HOME` env override (default `~/.ctp2`) so tests and alternate installs can target another directory.
- [ ] **Engine — `CivPaths.cpp` HOME-expansion.** After parsing `civpaths.txt`, if `$CTP2_HOME/original_data` exists, override `m_hdPath → $CTP2_HOME`, `m_dataPath → original_data` (all asset lookups resolve under `$CTP2_HOME/original_data/{default,english,…}`), save base → `$CTP2_HOME/saves` (replaces the current macOS `Application Support` branch + the non-Apple relative branch), scenarios under the same root. Legacy fallback: if absent, keep today's in-tree behavior so nothing breaks mid-migration. `CTP2_HOME` defaults to `$HOME/.ctp2`. **Unify `ModernSpriteAtlas` on the same `CTP2_HOME`** instead of its hardcoded `$HOME/.ctp2/assets/`.
- [ ] **Tests — `tools/assets/tests/` (pytest).** Synthetic `ctp2_data/` fixture → assert mirrored tree + idempotent re-run; `--purge` refuses on verification failure and deletes only after success. Sprite conversion stays out of unit tests (needs real `.SPR`); cover the new/fallible copy/purge/verify logic.
- [ ] **Delete in-tree `ctp2_data/`** once the game is verified running from `~/.ctp2` (via `--purge` or by hand).

**Modernization of *other* asset types — future, discuss before starting.** Only sprites have a converter today. Realistic end-state is **converted where a GPU format helps, verbatim for config/media** — not "everything converted":
- Tiles `.TIF` → texture/atlas — no converter yet; high value for GPU terrain.
- Pictures / icons / cursors → thin PNG/KTX2 re-encode (already near-standard images).
- `gamedata` / `uidata` (text tables, layouts, fonts) → config, not GPU assets; **stay verbatim** permanently.
- `sound` / `videos` → transcode possible but low-priority/format-risky; **stay verbatim**.
So `original_data/` remains the canonical source the converters read from; `assets/` grows as loaders land. The relocation sets this up without committing to any of it.

### Already-modern (verified 2026-07-12 — don't re-propose)

C++20 (`cpp_std=c++20`); arm64-native clang build; `hardening_level=maximum` dev default (+`-ftrapv`, `_GLIBCXX_ASSERTIONS`); UBSan smoke tier wired into `make ubsan-smoke`; five modernization ratchets enforced by `make test`; JSON saves (binary CivArchive path deleted); SDL2 + SDL2_mixer; A* node allocation already arena-pooled; 963-finding bug-hunt triaged with P1/P3 resolution blocks.

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

## Test Nets (what protects a change; expanded 2026-07-13)

- **Fast/pre-commit:** `make test` (ratchets + fast + unit doctests).
- **UB:** `make ubsan-smoke` (new game + turns under UBSan).
- **Integration:** `meson test -C build --suite integration` — slices (headless + real-window UI incl. the P11 pixel oracle), JSON round-trips, determinism, multi-turn AI (~7 min).
- **Scenario (9 tests):** `meson test -C build --suite scenario` — fixture reloads (`load-stress`), city capture, cargo/path resume, **combat-invariant matrix**, **seed sweep** (3 seeds CI), **undersea city** (nano-age via `create_unit`/`grant_advance` cheats), **conquest ending** (the game actually finishes), **long-game soak** (300 rounds + economy-corruption invariants + deep save/load at round 300).
- **Release tier (per milestone):** `make release-check` — the soak + a 20-seed × 60-turn sweep on the optimized binary (~1.5 min total); `make seed-sweep` for the sweep alone.
- **DEBUG cheat verbs for tests** (raw_cmd only, not MCP): `grant_advance <id|ADVANCE_*>`, `create_unit <UNIT_*|idx> <x> <y>` — reach any age's content in seconds.

## Last Known Verification Commands

```sh
mise exec -- make test
mise exec -- make ubsan-smoke
mise exec -- meson test -C build --suite scenario   # 9 gameplay nets
mise exec -- make release-check                     # per milestone, optimized tier
```

## Known Blockers / Caveats

- ASan hangs pre-`main` on this macOS/Apple clang setup (see Caveats); `make ubsan-smoke` is the working sanitizer path until P2 lands the Linux tier.
- The local CI daemon (`.ci/daemon.sh`) is **not running** and `.ci/state.json` is stale (last update 2026-06-07, branch `wave1-ctp2-namespace`). Restart it before relying on `jq '.tier_a.status' .ci/state.json`, or agents will read a month-old green.
- `BUG_HUNT_REPORT.md` has no fixed/open tracking — 963 findings, ~63 fixed in May 2026, no annotations since. P1's checkbox includes fixing that.
- Windows is best-effort and should not drive refactoring decisions unless Olek asks.
- Avoid `MBCHAR *` UI string-literal cleanup unless intentionally doing a broader UI const-correctness batch.
