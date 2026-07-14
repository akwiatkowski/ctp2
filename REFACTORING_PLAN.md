# CTP2 Refactoring Plan

Purpose: give any assistant or developer a quick, shared answer to: "how much more refactoring work remains?"

Plain Markdown so any LLM (OpenAI, Claude, local models) can read and update it. Keep it short. Update after each committed batch.

## Overall Progress

- **Original Definition of Done (M1–M8): COMPLETE and formally declared 2026-07-10.** The P7 close-out documentation pass has landed (the "Remaining Legacy-Risk Areas" consolidation below), and a final verification pass (`make test` + `make ubsan-smoke` + integration/scenario suites, clean worktree) is green. Nothing remains to declare the original DoD met.
- **Current phase: P11 GPU rendering Stage 2 + M10 modern-asset atlas, reprioritized 2026-07-14 ahead of the memory-safety phase by Olek.** Memory safety & stability remains open (P2 Linux ASan, P6 RAII), but P11/M10 is now the active path because finishing GPU rendering requires the modern-asset atlas substrate and the SDL3 backend migration.
- **Modernization Phase 2 scoped 2026-07-12** (analysis pass: modern / safe / fast-on-modern-machines): **P8** perf build tier — measured **5.3× wall / 8.6× CPU** from a release build (every existing tier was `-O0`+`-ftrapv`; outcome bit-identical, so optimization does not break determinism); **P9** container modernization (954 legacy `PointerList`/`DynamicArray`/`SimpleDynamicArray` uses); **P10** type-erased-cast burn-down (2487 after redundant file-I/O and binary-read cast cleanups); **P11** GPU rendering Stage 2 un-parked with M10 on 2026-07-14. Verified-already-modern list added so items don't get re-proposed (C++20, hardening max, UBSan tier, arena-pooled A*, JSON saves).
- **Progress:** P1 complete (all 54 CRITICAL verified). **P5 complete 2026-07-10** — JSON-load derived-cache audit: `SetAllMoveCost`/`CalcChokePoints` need no rebuild (outputs are dense-serialized per-cell); the only non-serialized world caches (continent size/neighbor arrays) were already rebuilt in `World::from_json`; one real gap found + fixed — stale `good_value` on a `ResourceDB`-size mismatch now recomputes via `ComputeGoodsValues`, pinned by a new regression test that fails without the fix. **P3 crash-class burned down 2026-07-02/03** — all 4 HIGH categories worked: DANGEROUS_SHIFT + DIVISION_BY_ZERO (`0 open` first-party), and MISSING_BOUNDS_CHECK + NULL_DEREFERENCE (`@~675`, `@~1206`; ~305 leads) verified in-code with resolution blocks — ~25 genuine crash bugs fixed, the rest already-fixed by the `g_player→safe_player` migration, false-positive, structurally-safe, or the supervised/P4/net tail. (A tier-drift found 2026-07-02, where earlier annotations landed in the MEDIUM mirror, was corrected.) **P4 effectively complete** — ratchet re-scoped to first-party (vendored libs excluded), then **85 real conversions drove `unsafe_string_api` 92 → 57** (the old libs-inclusive 1360 was 93% vendored noise); one real overflow bug caught (`tech_MemMap::GetFileExtension`). The clean pool is exhausted — the remaining 57 was dead code (~20), deferred `net/` (~22), and the off-limits `GetXName` idiom / hard cascades (~15); **the dead code was deleted 2026-07-12 (57 → 39, plus a live `GetLabel` `sizeof(ptr)` truncation bug found and fixed in the process — P4 now fully complete)**. P6 ticked 11/574 (17/574 as of 2026-07-11). Key finding: `BUG_HUNT_REPORT.md` HIGH findings are stale leads — line numbers drifted, most already fixed or false-positive; **verify by code pattern, not line number** — and **check the severity tier** before annotating.
- **Test-coverage expansion 2026-07-13** (from a coverage brainstorm; all six picked items shipped): scenario suite grew **5 → 9 tests** — seed sweep (suite no longer lives on seed 42), combat-invariant matrix, forced undersea city (new `create_unit` DEBUG verb; `grant_advance` now takes `ADVANCE_*` names), the **first test ever to finish a game** (conquest ending recognized), and deep-state save/load + economy-corruption invariants folded into the long-game soak. **The deep-state oracle caught a real crash on its first run** — in-process `load_game` of any save with live trade routes aborted in pool teardown (`Game::Set*Ptr` uses `unique_ptr::reset`, which installs the new pool *before* the old dtor runs, so `~TradePool`'s gameplay route-Kill cascade hit the fresh empty pool). Fixed at three layers: dtor cascade removed, `~CityData` route-kills guarded via null-safe `TradeRoute::IsValid`, trade-before-units pool re-init order (commit `d2e14d94`). Unpicked cheap wins: map-generator×wrap sweep (~1h) and a cross-tier determinism diff (debug vs release metrics = automatic UB net, technique proven during P8).
- **Un-parked 2026-07-14:** modern-asset converter (formerly M10), required for P11 Stage 2 atlas-backed GPU compositing.

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
- Modernization Phase 2 (P8–P10): **P8 is ~1 session with a measured 5.3×/8.6× payoff waiting; P9 multi-session; P10 opportunistic.** P11/GPU parked with M10.
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
| P11 | GPU-path rendering | **Stage 1 ✅ done** (pixel oracle + dirty-rect present, −73% upload bytes); Stage 2 un-parked 2026-07-14 with M10; SDL3 backend probe builds and passes fast+unit tests | staged |
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

| Counter | Count | Nature of the remainder |
| --- | --: | --- |
| `raw_new` | 4819 | RAII conversion (P6); ~413 medium files need per-file ownership analysis, no mechanical tier. (Includes ~11 `--new-game`-style string false positives absorbed in test files.) |
| `raw_delete` | 1865 | same P6 effort (paired with `raw_new`). |
| `c_allocation` | 101 | `malloc`/`calloc`/`realloc`/`free`; the smallest bucket, mostly in the supervised hard/very-hard tail. |
| `type_erased_casting` | 2487 | `void*`/C-cast handoffs; opportunistic cleanup only — large, cross-cutting, low crash-relevance. |
| `unsafe_string_api` | 57 | P4 floor — dead code (~20, deletable), deferred `net/` (~22), off-limits `GetXName` idiom + hard cascades (~15). |

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

**Stage 2 un-parked 2026-07-14 with M10 (reprioritized ahead of P2/P6 memory-safety work by Olek):** per-window/per-layer `SDL_Texture` GPU compositing, 32-bit world surface, and draw-time render flags (fog/desaturation) as GPU ops. The dirty-union seam + pixel oracle from Stage 1 are the foundation it builds on. Phase A is underway: the SDL primary/secondary screen surfaces are ARGB8888 and `ColorBlt`/chroma-key setup now support 32bpp, so the present path no longer needs a per-frame 565→8888 conversion for screen-surface clears/fills/transparency setup. General same-format `aui_Blitter::Blt`, `TileBlt`, and `BevelBlt` now handle the common 32bpp UI paths, covered by focused doctests and verified with `make test`, SDL3 `fast unit`, and sequential `slice-headless`/`slice-ui` (including the presented-frame pixel oracle). Direct SDL `Secondary()` writer audit: splash text routes through primitives/blitters, raw movie frame copies are DirectX-only, and the debug bitmap-font cache dump self-rejects non-16-bit surfaces.

### M10 — Modern-asset converter + first-run (un-parked 2026-07-14)

Offline converter (packed atlas + JSON manifests into `~/.ctp2/assets/<fingerprint>/`) plus an engine modern-first loader with legacy fallback. See `docs/modern-assets.md`. This now proceeds with P11 Stage 2 ahead of the remaining memory-safety phase because atlas-backed textures and the SDL3 backend are prerequisites for real GPU compositing and shader render flags.

**B1 started 2026-07-14:** `spr_export.py` has a pure Python LZW1 decoder wired into the v2 normal-frame parser, verified with synthetic copy-mode, compressed literal/back-reference streams, and a synthetic v2 frame payload because no licensed `.SPR` assets are committed. Real v2 parity remains the next B1 step and needs Olek's local data path.

**B2 started 2026-07-14:** `spr_export.py --atlas` now packs decoded v0/v1 unit frames into one dependency-free RGBA PNG atlas plus manifest rects using the existing decoder output. `--modern-assets` writes under `~/.ctp2/assets/<source-fingerprint>/`; when pointed at a directory it recursively walks `.SPR` files and fingerprints the source set from relative paths + contents. Both are verified by the synthetic self-test; real atlas parity still needs a local `.SPR` data path.

**B2 validator 2026-07-14:** `spr_export.py --validate-manifest <json>` checks atlas manifest self-consistency (atlas metadata, per-frame references, rect bounds). This is the loader-contract guard before any engine modern-first path trusts generated assets.

**Phase C seam started 2026-07-14:** the engine now has a small `ModernSpriteManifest` C++ parser/validator for generated atlas manifests, covered by fast doctests. It does not load textures or change rendering yet; it only establishes the validated data shape the modern-first loader will consume.

**Phase D SDL3 backend probe 2026-07-14:** local pkg-config has `sdl3=3.4.12` and `sdl3_mixer=3.2.4`. The `-Dsdl_backend=sdl3` Meson build now links `ctp2`, `ctp2_headless`, `ctp2_fast_tests`, and `ctp2_unit_tests`, with `meson test -C build-sdl3-probe fast unit` green. SDL2 remains the default backend.

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
