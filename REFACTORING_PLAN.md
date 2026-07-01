# CTP2 Refactoring Plan

Purpose: give any assistant or developer a quick, shared answer to: "how much more refactoring work remains?"

This file is deliberately plain Markdown so it is easy for different LLMs (OpenAI, Claude Opus, local models) to read and update. Keep it short. Update the board after each committed batch.

## Overall Progress

**35 / 45 checkboxes done (~78%).** Remaining bounded work: **about 4-8 focused sessions** (M7 finish + M8 + M9 close-out).

Recount any time with:

```sh
grep -c '^- \[x\]' REFACTORING_PLAN.md   # done
grep -c '^- \[ \]' REFACTORING_PLAN.md   # remaining
```

## Current Definition Of Done

The refactoring effort is "done enough" when CTP2 has:

- Fast pre-commit tests that stay green. ✅ (M1, M6)
- A working sanitizer smoke tier on macOS/Linux. ✅ (`make ubsan-smoke`; ASan documented as platform-blocked)
- Modernization ratchets preventing regression in unsafe patterns. ✅ (M3; enforced in `make test`)
- Warning noise reduced enough that new warnings are visible. ✅ (M2; leftovers documented)
- Timelapse/play-session tooling stable enough for repeated use. ✅ (M4)
- A first modern asset-conversion path that preserves original game data as canonical input. ⬜ (M8)
- A documented list of remaining legacy-risk areas instead of open-ended cleanup. ⬜ (M9)

This is not a full rewrite. A fully modernized CTP2 engine is open-ended and likely much larger than this project needs.

## Short Answer Formula

When asked "how much more work remains?", answer from this scale:

- Finish obsolete subsystem removal (M7): **1-3 sessions**.
- Modern asset pipeline spike (M8): **3-6 sessions**.
- Close-out documentation pass (M9): **~1 session**.
- Fully refactored engine: **open-ended / not a bounded goal**.

Recommended default answer: **about 5-9 focused sessions** to reach the Definition of Done.

## Milestone Board

Use this as the real burn-down list. Move an item to `[x]` only after the code is committed and the listed verification has passed.

| ID | Milestone | Status | Estimate | Verification |
| --- | --- | --- | --- | --- |
| M1 | Baseline safety loop | 4/4 done | complete | `make test`, `make ubsan-smoke` |
| M2 | Warning noise reduction | 8/8 done | complete | `make test`, warning category visibly reduced |
| M3 | Modernization ratchet burn-down | 6/6 done | complete | ratchet baseline lowered without regressions |
| M4 | Timelapse/play tooling polish | 5/5 done | complete | short timelapse smoke, docs updated |
| M5 | UI/frame polish | 3/3 done | complete | visible/manual or smoke verification |
| M6 | Final stabilization pass | 4/4 done | complete | all standard checks, plan updated |
| M7 | Obsolete subsystem removal | 5/8 done | 1-3 sessions | obsolete code removed without network/movie regressions |
| M8 | Modern asset pipeline spike | 0/5 done | 3-6 sessions | legacy sprite exports reproducibly; visual parity checked |
| M9 | Close-out | 0/2 done | ~1 session | plan reflects reality; DoD declared |

### M1: Baseline Safety Loop

- [x] Add modernization ratchets to `make test`.
- [x] Add `sanitized-smoke` target for ASan/UBSan attempt.
- [x] Add working `ubsan-smoke` target.
- [x] Document ASan macOS blocker in this plan with sample-stack summary.

ASan blocker summary: on the current macOS/Apple clang setup, `build-sanitized/ctp2_headless` hangs before `main` in the dynamic loader/ASan runtime. A `sample` of the process showed `dyld4::APIs::runAllInitializersForMain -> libSystem_initializer -> __malloc_init -> wrap_malloc_default_zone -> __asan::AsanInitFromRtl -> __asan::InitializeShadowMemory -> __sanitizer::MemoryRangeIsAvailable`, then spinning in `__sanitizer::StaticSpinMutex::LockSlow`. This means the ASan issue is currently platform/runtime startup, not CTP2 game initialization. Use `make ubsan-smoke` as the working sanitizer tier until ASan is fixed or tested on another platform/toolchain.

### M2: Warning Noise Reduction

- [x] Clean scalar `NULL` defaults/args in isolated files.
- [x] Clean first include-case and fixed-array-null warning batches.
- [x] Mechanical `Player.h` -> `player.h` include-case sweep for headers/sources that compile in fast/unit/headless paths.
- [x] Mechanical `CityData.h` -> `citydata.h` sweep where warnings remain.
- [x] Fix low-risk AI `&&`/`||` precedence warnings in `Goal.cpp` and `settlemap.cpp`.
- [x] Fix safe dead-local / unused-variable warnings where the variable has no side effect.
- [x] Fix simple constructor initializer-order warnings where member order is obvious.
- [x] Re-run `make ubsan-smoke` and record remaining high-signal warning categories here.

Remaining high-signal warning categories after the M2 pass (post-DoD, opportunistic only):

- Include-case stragglers in files outside the targeted fast/unit/headless sweep, mostly remaining `Player.h` includes pulled in by broader rebuilds.
- Legacy writable string conversions (`char *` / `MBCHAR *` from string literals), especially diplomacy, UI popup/window, and logging paths.
- Broader unused-variable / set-but-unused warnings in large legacy files such as `governor.cpp`, `ArmyData.cpp`, and `diplomat.cpp`; many need local behavior review before removal because initializers can have side effects.
- Additional logical precedence warnings in broad game/pathing files such as `ArmyData.cpp` and `robotastar2.cpp`; defer to smaller behavior-preserving batches.
- Switch exhaustiveness warnings for legacy enums and overloaded-virtual hiding warnings in UI/sprite/network classes.

### M3: Modernization Ratchet Burn-Down

- [x] Establish ratchet baseline and enforce it in `make test`.
- [x] Reduce one small unsafe string API cluster, then update baseline.
- [x] Reduce one small raw `new`/`delete` ownership cluster with clear ownership.
- [x] Reduce one C allocation cluster where lifetime is local/simple.
- [x] Reduce one type-erased cast cluster only if behavior is obvious.
- [x] Add a short note here listing ratchet counts after the last reduction.

Current baseline (`tools/modernization/ratchet_baseline.json`, verified 2026-07-02): `c_allocation=516`, `raw_delete=2065`, `raw_new=5528`, `type_erased_casting=3739`, `unsafe_string_api=1416`. Further reductions are post-DoD, opportunistic only.

### M4: Timelapse / Play Tooling Polish

- [x] Run long fogged hero timelapse successfully.
- [x] Add `query_names` metadata and richer Chronicle captions.
- [x] Add short usage docs for common timelapse commands.
- [x] Add a tiny caption-render smoke fixture or helper test if practical.
- [x] Pick one visible caption/frame polish improvement and verify with a short run.

### M5: UI / Frame Polish

- [x] Pick one concrete frame pacing/window persistence/UI scaling issue.
- [x] Implement the smallest visible fix.
- [x] Verify on desktop and avoid broad UI constructor const-correctness unless explicitly scoped.

M5 fix: the SDL main-loop idle cap now uses a 17 ms frame budget, matching `1000 / 60` rounded up instead of an undershooting 16 ms magic number. Verified with `mise exec -- make test`, `git diff --check`, and `mise exec -- make ubsan-smoke`; committed as `a3e59cf6`.

### M6: Final Stabilization Pass

- [x] Run `make test` and `make ubsan-smoke` cleanly after final batch.
- [x] Update this plan with final counts and remaining caveats.
- [x] Ensure `README.md` does not contradict current Makefile commands.
- [x] Stop with clean worktree and a concise handoff.

M6 final verification: `mise exec -- make test`, `git diff --check`, and `mise exec -- make ubsan-smoke` pass. `README.md` build instructions now point at the current root-level `mise exec -- make setup`, `mise exec -- make build`, `mise exec -- make test`, and UBSan smoke workflow instead of the stale `build-headless` Meson commands.

### M7: Obsolete Subsystem Removal

Scope: remove legacy systems that are no longer product goals. Windows support should later use SDL/Linux-like paths, not old DirectX/Win32 runtime plumbing. Networking is out of scope for this milestone. Movie playback and movie DB/schema/data are product goals and should be preserved for future repair, not removed.

- [x] Remove remaining CD-ROM / Redbook audio / copy-protection code.
- [x] Confirm GameWatch provenance; if it is original Activision telemetry/recording/plugin code, remove it.
- [x] Remove Windows registry / file-association / DirectX startup checks.
- [x] Guard or drop the unconditional `aui_directx` includes in active sources: `ui/aui_common/aui_Factory.cpp` (3 headers), `ui/aui_ctp2/c3blitter.cpp` (`aui_directsurface.h`), `ctp/civ3_main.cpp` (`aui_directmoviemanager.h`). Use branches (`c3ui.h` `#else` pattern) or `__AUI_USE_DIRECTX__` guards so SDL builds no longer need the headers on disk.
- [x] Decide movie-path handling before deletion: `aui_directmovie.*` / `aui_directmoviemanager.*` and the self-guarded `directvideo.*` are movie code — keep, relocate, or stub them (movies are a product goal for later repair).
- [ ] Delete the remaining `ui/aui_directx/` sources that are not needed for movie repair.
- [ ] Purge `aui_directx` references from Windows/legacy project files: `ui/ui.dsp`, `ui/ui.vcxproj(.filters)`, `ctp2_code/Makefile.am`, `gs/newdb/Makefile.am`, plus stragglers in `net/net.dsp`, `gs/gs.dsp`, `robot*/…dsp`, `mapgen/plasma1.dsp`.
- [ ] Re-run `make test`, `git diff --check`, and `make ubsan-smoke`; record any retained compatibility caveats here.

GameWatch was confirmed as obsolete plugin-based recording/delivery code (`gwciv`, `gwfile`, `gwarchive`) that wrote unit build/kill records and `recordN.dat` payloads. Runtime hooks, profile setting, active Meson include path, legacy autotools include paths, source tree, DLL, and static libraries were removed in `b4f79eb0`. Stale Visual Studio GameWatch include paths, import defines, and library dependencies were removed with the Windows registry / DirectX startup cleanup batch.

Windows registry / DirectX startup cleanup removed the `.c2g` file-association registry writes, the fatal `dxver.dll` startup gate, and the duplicate AUI `dxver` probe. Movie/DirectShow COM code and networking/anet registry helpers are intentionally retained: movies should be repaired later, and networking is outside this milestone.

DirectX AUI backend status (verified 2026-07-02): the active Meson build no longer references `ui/aui_directx` include paths or source files (`8eb6fe01`). The remaining unconditional `aui_directx` includes in active sources are now guarded (2026-07-02): `aui_Factory.cpp` and `c3blitter.cpp` wrap their DirectX headers in `#if defined(__AUI_USE_DIRECTX__)` (the symbols are only used in the matching `#elif defined(__AUI_USE_DIRECTX__)` branches), and `civ3_main.cpp` wraps `aui_directmoviemanager.h` in `#if !defined(__GNUC__)` to match the exact guard on the only `aui_DirectMovieManager` use. SDL builds (`make test`, `make ubsan-smoke`) no longer need the `ui/aui_directx` headers on disk. The `ui/aui_directx/` source tree still exists; deleting it (after settling movie-path handling) remains a later M7 checkbox. `directvideo.*` stays in the Meson build but is fully self-guarded by `__AUI_USE_DIRECTX__`, so it compiles to nothing on SDL.

Movie-path handling decision (2026-07-02): **keep movie code in place, guarded; do not relocate or stub in this refactor.** Rationale and consequences for the deletion checkbox:

- Build config: `auicfg.h` defines `__AUI_USE_DIRECTX__` + `__AUI_USE_DIRECTMEDIA__` only under `WIN32`; SDL builds get `__AUI_USE_SDL__` only. So all DirectX/DirectMedia movie code is inert on the current macOS/Linux build.
- `ui/aui_directx/aui_directmovie.*` and `aui_directmoviemanager.*` are the DirectShow reference implementation, guarded by `__AUI_USE_DIRECTMEDIA__` and absent from the Meson build. Keep them as the thing to repair later. The only active-source touch is `civ3_main.cpp`, whose include and `new aui_DirectMovieManager()` use are both guarded (`!__GNUC__`).
- The cross-platform movie surface already compiles in the SDL build and stays: `ui/aui_common/aui_movie.*` / `aui_moviemanager.*` / `aui_moviebutton.*`, `ui/aui_ctp2/directvideo.*` (self-guarded by `__AUI_USE_DIRECTX__`), `gs/database/moviedb.*`, and the `ui/interface/*moviewin*` windows.
- Consequence for the next checkbox: `aui_directmovie.cpp` includes sibling headers `aui_directui.h` and `aui_directsurface.h`, so those two headers count as "needed for movie repair" and must be retained (or their loss explicitly accepted as dangling includes) when the rest of `ui/aui_directx/` is deleted.

Do not remove yet:

- Network / multiplayer code; it will be resolved later.
- Movie playback code and wonder/victory movie DB/schema/data fields; the desired direction is to make movies work later.
- `aui_directmovie.*`, `aui_directmoviemanager.*`, and their `aui_directui.h` / `aui_directsurface.h` dependencies within `ui/aui_directx/` (needed for movie repair per the decision above).

### M8: Modern Asset Pipeline Spike

Goal: make original game data compatible with a future modern renderer without breaking mod/original-data compatibility. Original assets remain canonical; generated modern assets are cache/build artifacts.

- [ ] Write a small `.SPR` inspector/exporter for one representative unit sprite (`GU###.SPR`).
- [ ] Export frames with action/facing/frame metadata, hot points, dimensions, and draw flags needed by the current renderer.
- [ ] Generate a debug-friendly PNG frame dump first; atlas/KTX-style packing can follow after parity is proven.
- [ ] Add a visual-parity check against the current CPU sprite path for one sprite/action/facing/frame set.
- [ ] Document the intended cache layout (`cache/assets/<data-hash>/...`) and fallback rule: load generated assets when valid, otherwise use legacy loaders.

### M9: Close-Out

- [ ] Write a "Remaining Legacy-Risk Areas" section in this file consolidating: leftover warning categories (M2 note), ratchet counts (M3 note), deferred systems (network, movies, Windows project files), and any M7/M8 caveats.
- [ ] Final verification pass (`make test`, `make ubsan-smoke`, clean worktree); update Overall Progress and declare the Definition of Done met.

## Progress Rules

- A normal `continue` should complete one checkbox or one coherent part of a checkbox.
- Update the Overall Progress counts and the Milestone Board `Status` column in the same commit as the checkbox change.
- If a task discovers a blocker, add or update a blocker/caveat instead of pretending progress happened.
- Prefer many small commits over one broad refactor commit.

## Next Best Batches

Do these in order unless Olek changes priorities:

1. M7: guard the unconditional `aui_directx` includes (smallest safe SDL-first slice).
2. M7: settle movie-code handling, then delete the rest of `ui/aui_directx/` and purge project-file references.
3. M8: start with a read-only `.SPR` inspector/exporter before changing runtime rendering.
4. M9: close-out documentation pass.

## Assistant Protocol

For any LLM continuing this work:

- Read this file first, then `git log --oneline -10`, then `git status --short`.
- Prefer one small batch per commit.
- Verify with `mise exec -- make test` and `git diff --check`.
- Use `mise exec -- make ubsan-smoke` after touching headless/game-loop code.
- Do not treat "fully refactored" as the goal unless Olek explicitly redefines scope.
- Do not make converted assets canonical; preserve original data/mod compatibility and treat generated modern assets as rebuildable cache/output.
- Warning cleanup and ratchet reductions are post-DoD opportunistic work now (M2/M3 complete); do them only as small, obvious side batches.

## Last Known Verification Commands

```sh
mise exec -- make test
mise exec -- make ubsan-smoke
```

## Known Blockers / Caveats

- `make sanitized-smoke` exists, but ASan currently hangs before `main` on this macOS/Apple clang setup; see M1 stack summary.
- `make ubsan-smoke` is the working sanitizer smoke path.
- Windows is best-effort and should not drive refactoring decisions unless Olek asks.
- Avoid `MBCHAR *` UI string-literal cleanup unless intentionally doing a broader UI const-correctness batch.
