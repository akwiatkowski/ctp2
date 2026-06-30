# CTP2 Refactoring Plan

Purpose: give any assistant or developer a quick, shared answer to: "how much more refactoring work remains?"

This file is deliberately plain Markdown so it is easy for different LLMs (OpenAI, Claude Opus, local models) to read and update. Keep it short. Update the table after each committed batch.

## Current Definition Of Done

The refactoring effort is "done enough" when CTP2 has:

- Fast pre-commit tests that stay green.
- A working sanitizer smoke tier on macOS/Linux.
- Modernization ratchets preventing regression in unsafe patterns.
- Warning noise reduced enough that new warnings are visible.
- Timelapse/play-session tooling stable enough for repeated use.
- A documented list of remaining legacy-risk areas instead of open-ended cleanup.

This is not a full rewrite. A fully modernized CTP2 engine is open-ended and likely much larger than this project needs.

## Work Remaining Estimate

| Track | Current State | Done When | Estimate |
| --- | --- | --- | --- |
| Commit hygiene | Recent batches committed; worktree should usually be clean | One logical commit per batch; no uncommitted drift | Ongoing |
| Fast test loop | `make test` runs ratchets + fast/unit tests | Stays under practical pre-commit time and is trusted | 0-1 sessions |
| Sanitizer smoke | `make ubsan-smoke` works; ASan hangs before `main` on current macOS setup and is documented below | UBSan smoke documented and ASan either fixed or explicitly marked platform-blocked | 0-2 sessions |
| Warning cleanup | Several low-risk batches done; many legacy warnings remain | High-signal warnings fixed: include case, precedence, scalar NULL, dead locals | 4-8 sessions |
| Modernization ratchets | Ratchets exist for raw new/delete, unsafe strings, C allocation, casts | Ratchets kept green and lowered after each intentional cleanup batch | 4-8 sessions |
| Timelapse tooling | Fogged hero timelapse works; captions have names metadata | Captions are useful, short runs are reproducible, docs explain common commands | 1-3 sessions |
| UI/frame polish | Not started in this sequence | Pick 1-2 visible polish wins, not a full UI rewrite | 2-4 sessions |
| Deeper architecture cleanup | Large legacy systems still coupled | Only targeted cleanup with tests; no broad rewrite | Open-ended |

## Short Answer Formula

When asked "how much more work remains?", answer from this scale:

- Minimum useful cleanup: **4-6 sessions**.
- Solid modernization pass: **10-16 sessions**.
- Mostly quiet warnings + sanitizer/ratchet discipline: **16-24 sessions**.
- Fully refactored engine: **open-ended / not a bounded goal**.

Recommended default answer: **about 10-16 focused sessions** to reach a solid, practical refactoring milestone.

## Milestone Board

Use this as the real burn-down list. Move an item to `[x]` only after the code is committed and the listed verification has passed.

| ID | Milestone | Status | Estimate | Verification |
| --- | --- | --- | --- | --- |
| M1 | Baseline safety loop | 4/4 done | complete | `make test`, `make ubsan-smoke` |
| M2 | Warning noise reduction | 2/8 done | 4-6 sessions | `make test`, warning category visibly reduced |
| M3 | Modernization ratchet burn-down | 1/6 done | 4-8 sessions | ratchet baseline lowered without regressions |
| M4 | Timelapse/play tooling polish | 2/5 done | 1-3 sessions | short timelapse smoke, docs updated |
| M5 | UI/frame polish | 0/3 done | 2-4 sessions | visible/manual or smoke verification |
| M6 | Final stabilization pass | 0/4 done | 2-3 sessions | all standard checks, plan updated |

### M1: Baseline Safety Loop

- [x] Add modernization ratchets to `make test`.
- [x] Add `sanitized-smoke` target for ASan/UBSan attempt.
- [x] Add working `ubsan-smoke` target.
- [x] Document ASan macOS blocker in this plan with sample-stack summary.

ASan blocker summary: on the current macOS/Apple clang setup, `build-sanitized/ctp2_headless` hangs before `main` in the dynamic loader/ASan runtime. A `sample` of the process showed `dyld4::APIs::runAllInitializersForMain -> libSystem_initializer -> __malloc_init -> wrap_malloc_default_zone -> __asan::AsanInitFromRtl -> __asan::InitializeShadowMemory -> __sanitizer::MemoryRangeIsAvailable`, then spinning in `__sanitizer::StaticSpinMutex::LockSlow`. This means the ASan issue is currently platform/runtime startup, not CTP2 game initialization. Use `make ubsan-smoke` as the working sanitizer tier until ASan is fixed or tested on another platform/toolchain.

### M2: Warning Noise Reduction

- [x] Clean scalar `NULL` defaults/args in isolated files.
- [x] Clean first include-case and fixed-array-null warning batches.
- [ ] Mechanical `Player.h` -> `player.h` include-case sweep for headers/sources that compile in fast/unit/headless paths.
- [ ] Mechanical `CityData.h` -> `citydata.h` sweep where warnings remain.
- [ ] Fix low-risk AI `&&`/`||` precedence warnings in `Goal.cpp` and `settlemap.cpp`.
- [ ] Fix safe dead-local / unused-variable warnings where the variable has no side effect.
- [ ] Fix simple constructor initializer-order warnings where member order is obvious.
- [ ] Re-run `make ubsan-smoke` and record remaining high-signal warning categories here.

### M3: Modernization Ratchet Burn-Down

- [x] Establish ratchet baseline and enforce it in `make test`.
- [ ] Reduce one small unsafe string API cluster, then update baseline.
- [ ] Reduce one small raw `new`/`delete` ownership cluster with clear ownership.
- [ ] Reduce one C allocation cluster where lifetime is local/simple.
- [ ] Reduce one type-erased cast cluster only if behavior is obvious.
- [ ] Add a short note here listing ratchet counts after the last reduction.

### M4: Timelapse / Play Tooling Polish

- [x] Run long fogged hero timelapse successfully.
- [x] Add `query_names` metadata and richer Chronicle captions.
- [ ] Add short usage docs for common timelapse commands.
- [ ] Add a tiny caption-render smoke fixture or helper test if practical.
- [ ] Pick one visible caption/frame polish improvement and verify with a short run.

### M5: UI / Frame Polish

- [ ] Pick one concrete frame pacing/window persistence/UI scaling issue.
- [ ] Implement the smallest visible fix.
- [ ] Verify on desktop and avoid broad UI constructor const-correctness unless explicitly scoped.

### M6: Final Stabilization Pass

- [ ] Run `make test` and `make ubsan-smoke` cleanly after final batch.
- [ ] Update this plan with final counts and remaining caveats.
- [ ] Ensure `README.md` does not contradict current Makefile commands.
- [ ] Stop with clean worktree and a concise handoff.

## Progress Rules

- A normal `continue` should complete one checkbox or one coherent part of a checkbox.
- Decrease the session estimate only after a committed, verified checkbox materially reduces remaining work.
- If a task discovers a blocker, add or update a blocker/caveat instead of pretending progress happened.
- Prefer many small commits over one broad refactor commit.

## Next Best Batches

Do these in order unless Olek changes priorities:

1. Run one more small warning cleanup batch from `make ubsan-smoke` output.
2. Normalize `Player.h` include casing in a mechanical batch, then verify.
3. Fix remaining low-risk precedence warnings in AI code.
4. Lower one modernization ratchet category intentionally, then update baseline.
5. Add a short timelapse usage note once captions are good enough.

## Assistant Protocol

For any LLM continuing this work:

- Read this file first, then `git log --oneline -10`, then `git status --short`.
- Prefer one small batch per commit.
- Verify with `mise exec -- make test` and `git diff --check`.
- Use `mise exec -- make ubsan-smoke` after touching headless/game-loop code.
- Do not treat "fully refactored" as the goal unless Olek explicitly redefines scope.
- After each committed batch, update the estimate table only if the estimate materially changed.

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
